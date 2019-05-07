#ifndef __MYOS__HARDWARECOMMUNICATION__PCI_H
#define __MYOS__HARDWARECOMMUNICATION__PCI_H

#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/port.h>
#include <hardwarecommunication/interrupts.h>

/* 
 * $ lspci -n
 * 00:00.0 0600: 8086:29a0 (rev 02)
 *               <vender_id>:<device_id>
 *
 * $ lspci -x
 * 00:00.0 Host bridge: Intel Corporation 82P965/G965 Memory Controller Hub (rev 02)
 * 00: 86 80 a0 29 06 01 90 00 02 00 00 06 00 00 00 00
 * 10: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 20: 00 00 00 00 00 00 00 00 00 00 00 00 b8 1a 00 04
 * 30: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 *
 */

namespace myos {

  namespace hardwarecommunication {

    class PeripheralComponentInterconnectDeviceDescriptor {
      public:
        myos::common::uint32_t portBase;
        myos::common::uint32_t interrupt;

        myos::common::uint16_t bus;
        myos::common::uint16_t device;
        myos::common::uint16_t function;

        myos::common::uint16_t vendor_id;
        myos::common::uint16_t device_id;

        myos::common::uint8_t class_id;
        myos::common::uint8_t subclass_id;
        myos::common::uint8_t interface_id;

        myos::common::uint8_t revision;

        PeripheralComponentInterconnectDeviceDescriptor();
        ~PeripheralComponentInterconnectDeviceDescriptor();
    };

    class PeripheralComponentInterconnectController {
        Port32Bit dataPort;
        Port32Bit commandPort;

      public:
        PeripheralComponentInterconnectController();
        ~PeripheralComponentInterconnectController();

        myos::common::uint32_t Read(myos::common::uint16_t bus, myos::common::uint16_t device, myos::common::uint16_t function, myos::common::uint32_t registeroffset);
        void Write(myos::common::uint16_t bus, myos::common::uint16_t device, myos::common::uint16_t function, myos::common::uint32_t registeroffset, myos::common::uint32_t value);
        bool DeviceHasFunctions(myos::common::uint16_t bus, myos::common::uint16_t device);

        void SelectDrivers(myos::drivers::DriverManager* driverManager);

        PeripheralComponentInterconnectDeviceDescriptor GetDeviceDescriptor(myos::common::uint16_t bus, myos::common::uint16_t device, myos::common::uint16_t function);
    };

  }

}

#endif
