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

void PeripheralComponentInterconnectController::SelectDrivers(DriverManager* driverManager, InterruptManager* interrupts) {
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

        // iterate the base address register
        for (int barNum = 0; barNum < 6; barNum++) {
          BaseAddressRegister bar = GetBaseAddressRegister(bus, device, function, barNum);

          // InputOutput mode
          // If we have an InputOutput BaseAddressRegister and the address is set
          // then we will set this port base value from device descriptor
          if (bar.address && (bar.type == InputOutput)) {
            dev.portBase = (uint32_t)bar.address;
          }

          // if we actually get a driver then we will add the driver to the driverManager
          Driver* driver = GetDriver(dev, interrupts);
          if (driver != 0) {
            driverManager->AddDriver(driver);
          }


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

// we will add a methods that will give us an instance of the space address register class
BaseAddressRegister PeripheralComponentInterconnectController::GetBaseAddressRegister(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar) {
  BaseAddressRegister result;

  uint32_t headertype = Read(bus, device, function, 0x0E) & 0x7F;
  int maxBARs = 6 - (4 * headertype);

  // if we have requested a place address register which is behind this maximun
  // then we just return the result which has not been set to anything legal
  // so the addresses now in this point and then the loop in the other function
  // you just have do nothing
  if (bar >= maxBARs) {
    return result;
  }

  // we read the offset `0x10 + 4 * bar` this bar number
  // because a base address register does have a size of 4
  // so this is a offset of the bars address register
  //
  // For Example:
  //  $ lspci -x
  //  00:00.0 Host bridge: Intel Corporation 82P965/G965 Memory Controller Hub (rev 02)
  //  00: 86 80 a0 29 06 01 90 00 02 00 00 06 00 00 00 00
  //  10:[00 00 00 00]00 00 00 00 00 00 00 00 00 00 00 00
  //       First        Second       Third      Fourth   Base Address Register(BAR)
  //
  uint32_t bar_value = Read(bus, device, function, 0x10 + 4 * bar);

  // examine the last bit
  // if it's 1 then InputOutput otherwise MemoryMapping;
  result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping;

  uint32_t temp;

  // Memory Space BAR Layout:
  // 31                         4 3          3 2  1 0      0
  // 16-Byte Aligned Base Address	Prefetchable Type Always 0
  if (result.type == MemoryMapping) {


    // get the base address register type
    // look is it 0, 1, or 2
    switch ((bar_value >> 1) & 0x3) {

      // the device will no accept writing all these (4 - 31 bit) to 1
      // if you set all (4 - 31 bit) to 1, it has the effect that
      // the device will cancel some of them (maybe 4 - 9 have zero bit instead)
      // and some zeros in the front also
      // so this tells you which of these bit are actually writable
      //
      // so you write all once and then you read the back and where you have a zero
      // then you cannot write anything
      //
      // this is an important information because the zeros up there
      // determine a limit on how big this memory mapping is allowed to be
      //
      // 31                         4 3          3 2  1 0      0
      // 16-Byte Aligned Base Address	Prefetchable Type Always 0
      // 
      // if let's say if the first 16 bits (31 - 16) are all at zero
      // then it means that you cannot have more than 64 kilobytes large area for memory mapping
      // because you have only 16 bits left and with 16 bits you can only address 65536 different memory locations
      //
      // on the other side the zeros down here (4 - ?)
      // the manufacturers of devices often use these bits for somthing else
      // these zeros tell you well the memory location that
      // I'm using for the memory mapping must start at an offset that is a multiple of 16 of 32 or whaterer
      case 0:  // 32 Bit Mode
        break;
      case 1:  // 20 Bit Mode
        break;
      case 2:  // 64 Bit Mode
        break;
    }

    result.prefetchable = ((bar_value >> 3) & 0x1) == 0x1;
  }

  // I/O Space BAR Layout
  // 31                        2 1      1 0      0
  // 4-Byte Aligned Base Address Reserved Always 1
  else { // InputOutput
    // set the address to the bar_value but cancel the last two bit
    result.address = (uint8_t*)(bar_value & ~0x3);

    // no prefetchable
    result.prefetchable = false;
  }

  return result;
}

Driver* PeripheralComponentInterconnectController::GetDriver(PeripheralComponentInterconnectDeviceDescriptor dev, InterruptManager* interrupts) {

  // In reality of course you would have some version of
  // if you have access to the hard drvies
  // then you would just read from the hard drive a file which should have a name that corresponds to these values
  // but we don't have access to the hard drive yet
  // so what we will do is we will hard code the driver into the kernel
  //
  switch (dev.vendor_id) {
    case 0x1022: // AMD
      switch(dev.device_id) {
        case 0x2000: // am79c973 driver
          printf("AMD am79c973 ");
          break;
      }
      break;

    case 0x8086: // Intel
      break;
  }

  // if we don't find the driver that is fitted for the particular device
  // then we might go into the generial devices
  // so if we have a class_id of 0x3, it's a graphics device,
  // and then we look at the subclass
  switch (dev.class_id) {
    case 0x03: // graphics
      switch (dev.subclass_id) {
        case 0x00: // VGA graphics devices
          printf("VGA ");
          break;
      }
      break;
  }

  return 0;
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
