#include <drivers/ata.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;

void printf(char*);

AdvancedTechnologyAttachment::AdvancedTechnologyAttachment(common::uint16_t portBase, bool master)
: dataPort(portBase),
  errorPort(portBase + 1),
  sectorCountPort(portBase + 2),
  lbaLowPort(portBase + 3),
  lbaMidPort(portBase + 4),
  lbaHiPort(portBase + 5),
  devicePort(portBase + 6),
  commandPort(portBase + 7),
  controlPort(portBase + 0x206)
{
  bytesPerSector = 512;
  this->master = master;
}

AdvancedTechnologyAttachment::~AdvancedTechnologyAttachment() {

}

void AdvancedTechnologyAttachment::Identify() {
  // want to talk to master or slave
  devicePort.Write(master ? 0xA0 : 0xB0);
  controlPort.Write(0);

  // read the status
  devicePort.Write(0xA0);
  uint8_t status = commandPort.Read();

  if (status == 0xFF) {
    return; // no device on the bus
  }

  // identify device
  // select which drive you want to talk to
  devicePort.Write(master ? 0xA0 : 0xB0);

  // how many sectors you want to read or write
  controlPort.Write(0);

  // the number of the sector you want to read or write
  lbaLowPort.Write(0);
  lbaMidPort.Write(0);
  lbaHiPort.Write(0);

  // give a read or write command
  commandPort.Write(0xEC);

  // then wait until the device is read
  status = commandPort.Read();
  if (status == 0x00) {
    return; // no device again
  }

  // it might take some time until the hard drive is ready to
  // give us the answer to this identify command, so here just wait
  // until the device ready
  while (((status & 0x80) == 0x80) // if device is busy
      && ((status & 0x01) != 0x01)) { // if device has error
    status = commandPort.Read();
  }

  if (status & 0x01) {
    printf("ERROR");
    return;
  }

  // data is ready and now we can read a sector, 512 bytes
  for (uint16_t i = 0; i < 256; i++) {
    // read the data and just print the data that we get
    uint16_t data = dataPort.Read();
    char *foo = "  \0";
    foo[1] = (data >> 8) & 0x00FF;
    foo[1] = data & 0x00FF;
    printf(foo);
  }
}

void AdvancedTechnologyAttachment::Read28(common::uint32_t sector) {

}

void AdvancedTechnologyAttachment::Write28(common::uint32_t sector, common::uint8_t* data, int count) {

}

void AdvancedTechnologyAttachment::Flush() {

}

