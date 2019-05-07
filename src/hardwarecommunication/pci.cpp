#include <hardwarecommunication/pci.h>

using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;

PeripheralComponentInterconnectDeviceDescriptor::PeripheralComponentInterconnectDeviceDescriptor() {

}

PeripheralComponentInterconnectDeviceDescriptor::~PeripheralComponentInterconnectDeviceDescriptor() {

}

PeripheralComponentInterconnectController::PeripheralComponentInterconnectController()
: dataPort(0xCFC),   // CONFIG_DATA
  commandPort(0xCF8) // CONFIG_ADDRESS
{

}

PeripheralComponentInterconnectController::~PeripheralComponentInterconnectController() {

}

/*
 * 31 	    31 | 30    24 |	23     16	 | 15         11 | 10            8 | 7             0
 * ------------+----------+------------+--------------------------------------------------
 *  Enable Bit | Reserved | Bus Number | Device Number | Function Number | Register Offset
 */

uint32_t PeripheralComponentInterconnectController::Read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset) {
  uint32_t id = (0x1 << 31)
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffset & 0xFC);

  commandPort.Write(id);
  uint32_t result = dataPort.Read();

  return result >> (8 * (registeroffset % 4));
}

void PeripheralComponentInterconnectController::Write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value) {

  uint32_t id = (0x1 << 31)
    | ((bus & 0xFF) << 16)
    | ((device & 0x1F) << 11)
    | ((function & 0x07) << 8)
    | (registeroffset & 0xFC);

  commandPort.Write(id);
  dataPort.Write(value);
}

bool PeripheralComponentInterconnectController::DeviceHasFunctions(uint16_t bus, uint16_t device) {
  return Read(bus, device, 0, 0x0E) & (1 << 7);
}

void printf(char*);
void printfHex(uint8_t);

void PeripheralComponentInterconnectController::SelectDrivers(DriverManager* driverManager) {
  for (int bus = 0; bus < 8; bus++) {

    for (int device = 0; device < 32; device++) {

      int numFunctions = DeviceHasFunctions(bus, device) ? 8 : 1;
      for (int function = 0; function < numFunctions; function++) {

        PeripheralComponentInterconnectDeviceDescriptor dev = GetDeviceDescriptor(bus, device, function);

        // one device can actually have a gap between functions
        // so it can have function 1 and function 5, but no function 2 and 3
        // `break` statement prevent us from finding the functions that behind after this gap
        // so we have to make this `continue` instead
        if (dev.vendor_id == 0x0000 || dev.vendor_id == 0xFFFF) {
          continue;
        }

        printf("PCI BUS ");
        printfHex(bus & 0xFF);

        printf(", DEVICE ");
        printfHex(device & 0xFF);

        printf(", FUNCTION ");
        printfHex(function & 0xFF);

        printf(" = VENDOR ");
        printfHex((dev.vendor_id & 0xFF00) >> 8);
        printfHex(dev.vendor_id & 0xFF);

        printf(", DEVICE ");
        printfHex((dev.device_id & 0xFF00) >> 8);
        printfHex(dev.device_id & 0xFF);

        printf("\n");
      }
    }
  }
}

PeripheralComponentInterconnectDeviceDescriptor PeripheralComponentInterconnectController::GetDeviceDescriptor(uint16_t bus, uint16_t device, uint16_t function) {
  PeripheralComponentInterconnectDeviceDescriptor result;

  result.bus = bus;
  result.device = device;
  result.function = function;

  result.vendor_id = Read(bus, device, function, 0x00);
  result.device_id = Read(bus, device, function, 0x02);

  result.class_id = Read(bus, device, function, 0x0B);
  result.subclass_id = Read(bus, device, function, 0x0A);
  result.interface_id = Read(bus, device, function, 0x09);

  result.revision = Read(bus, device, function, 0x08);
  result.interrupt = Read(bus, device, function, 0x3C);

  return result;
}
