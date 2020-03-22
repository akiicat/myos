#include <drivers/ata.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;

void printf(char*);
void printfHex(uint8_t);

AdvancedTechnologyAttachment::AdvancedTechnologyAttachment(bool master, common::uint16_t portBase)
: dataPort(portBase),
  errorPort(portBase + 0x1),
  sectorCountPort(portBase + 0x2),
  lbaLowPort(portBase + 0x3),
  lbaMidPort(portBase + 0x4),
  lbaHiPort(portBase + 0x5),
  devicePort(portBase + 0x6),
  commandPort(portBase + 0x7),
  controlPort(portBase + 0x206)
{
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
  sectorCountPort.Write(0);

  // the number of the sector you want to read or write
  lbaLowPort.Write(0);
  lbaMidPort.Write(0);
  lbaHiPort.Write(0);

  // give a read or write command
  commandPort.Write(0xEC); // identify command

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
  for (int i = 0; i < 256; i++) {
    // read the data and just print the data that we get
    uint16_t data = dataPort.Read();
    char *text = "  \0";
    text[0] = (data >> 8) & 0xFF;
    text[1] = data & 0xFF;
    printf(text);
  }
  printf("\n");
}

void AdvancedTechnologyAttachment::Read28(common::uint32_t sectorNum, int count) {
  // you cannot write to a sector larger than what you can address with 28 bits here
  // so we need to check the first four bits are zero
  if (sectorNum > 0x0FFFFFFF) {
    return;
  }

  // reading and writing in 28 bit mode
  // so what we do is put these into the device port
  devicePort.Write((master ? 0xE0 : 0xF0) | ((sectorNum & 0x0F000000) >> 24));

  // clear the previous error message
  errorPort.Write(0);

  // always read or write a single sector
  sectorCountPort.Write(1);

  // split the sector number and put it into these 3 ports
  // so that gives you only 24 bits to send and four bits are left from the address
  lbaLowPort.Write(  sectorNum & 0x000000FF );
  lbaMidPort.Write( (sectorNum & 0x0000FF00) >> 8);
  lbaLowPort.Write( (sectorNum & 0x00FF0000) >> 16 );

  // give a read or write command
  commandPort.Write(0x20);

  // if we write, we don't have to wait. Just say we want to write and we send the data
  // and the hard drive buffer that data and it's not our problem anymore
  //
  // but when we want to read data then it might take some time until the hard drive
  // is ready to give us that data.
  //
  // Wait for the device to be ready to give us a data
  uint8_t status = commandPort.Read();
  while (((status & 0x80) == 0x80) // if device is busy
      && ((status & 0x01) != 0x01)) { // if device has error
    status = commandPort.Read();
  }

  if (status & 0x01) {
    printf("ERROR");
    return;
  }

  printf("Reading ATA Drive: ");

  for (int i = 0; i < count; i += 2) {
    // we want to write data to this data array
    uint16_t wdata = dataPort.Read();

    // print the data before we send it
    char *text = "  \0";
    text[0] = wdata & 0xFF;
    if(i+1 < count) {
      text[1] = (wdata >> 8) & 0xFF;
    }
    else {
      text[1] = '\0';
    }

    printf(text);
  }

  // read a full sector
  for (int i = count + (count%2); i < 512; i += 2) {
    dataPort.Read();
  }

}

void AdvancedTechnologyAttachment::Write28(common::uint32_t sectorNum, common::uint8_t* data, common::uint32_t count) {
  // you cannot write to a sector larger than what you can address with 28 bits here
  // so we need to check the first four bits are zero
  if (sectorNum > 0x0FFFFFFF) {
    return;
  }

  // this is only to write one sector 
  // if you try to write too much, just refuse that
  if (count > 512) {
    return;
  }

  // reading and writing in 28 bit mode
  // so what we do is put these into the device port
  devicePort.Write((master ? 0xE0 : 0xF0) | ((sectorNum & 0x0F000000) >> 24));

  // clear the previous error message
  errorPort.Write(0);

  // always read or write a single sector
  sectorCountPort.Write(1);

  // split the sector number and put it into these 3 ports
  // so that gives you only 24 bits to send and four bits are left from the address
  lbaLowPort.Write(  sectorNum & 0x000000FF );
  lbaMidPort.Write( (sectorNum & 0x0000FF00) >> 8);
  lbaLowPort.Write( (sectorNum & 0x00FF0000) >> 16 );

  // give a read or write command
  commandPort.Write(0x30);

  printf("Writing to ATA: ");

  // write the number of bytes in this data
  for (int i = 0; i < count; i += 2) {
    // take the first bits from the data array here
    uint16_t wdata = data[i];

    // and the next 8 bits
    if (i + 1 < count) {
      wdata |= ((uint16_t)data[i+1]) << 8;
    }
    // the device always expects us to send as many bytes as in a sector
    // so we always have to write a full sector
    // otherwise we would get an interrupt with an error message
    dataPort.Write(wdata);

    // print the data
    char *text = "  \0";
    text[0] = (wdata >> 8) & 0xFF;
    text[1] = wdata & 0xFF;
    printf(text);
  }

  // if we write less than the 512 bytes, we filled the reset of the sector with zeros
  // but if count is an odd number at this point, first one has already been written
  // inside the above loop with empty value was 0 for that bytes
  // so in that case just to cover that.
  // Add count modulus 2 to the initialize
  for (int i = count + (count%2); i < 512; i += 2) {
    dataPort.Write(0x0000);
  }
}

void AdvancedTechnologyAttachment::Flush() {

  // take master or slave
  devicePort.Write(master ? 0xE0 : 0xF0);

  // flush command: 0xE7
  commandPort.Write(0xE7);

  // we didn't wait before after the writing but now have to wait device is flushing
  uint8_t status = commandPort.Read();
  if (status == 0x00) {
    return;
  }

  while (((status & 0x80) == 0x80) // if device is busy
      && ((status & 0x01) != 0x01)) { // if device has error
    status = commandPort.Read();
  }

  if (status & 0x01) {
    printf("ERROR");
    return;
  }


}

