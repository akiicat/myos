#include <drivers/amd_am79c973.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;

amd_am79c973* backend;

RawDataHandler::RawDataHandler(amd_am79c973* backend) {
  this->backend = backend;
  backend->SetHandler(this);
}

RawDataHandler::~RawDataHandler() {
  backend->SetHandler(0);
}

bool RawDataHandler::OnRawDataReceived(uint8_t* buffer, uint32_t size) {
  return false;
}

void RawDataHandler::Send(uint8_t* buffer, uint32_t size) {
  backend->Send(buffer, size);
}

void printf(char*);
void printfHex(uint8_t);

amd_am79c973::amd_am79c973(PeripheralComponentInterconnectDeviceDescriptor* dev, InterruptManager* interrupts)
: Driver(),
  InterruptHandler(interrupts, dev->interrupt + interrupts->HardwareInterruptOffset()),
  MACAddress0Port(dev->portBase),
  MACAddress2Port(dev->portBase + 0x2),
  MACAddress4Port(dev->portBase + 0x4),
  registerDataPort(dev->portBase + 0x10),
  registerAddressPort(dev->portBase + 0x12),
  resetPort(dev->portBase + 0x14),
  busControlRegisterDataPort(dev->portBase + 0x16)
{
  this->handler = 0;
  currentSendBuffer = 0;
  currentRecvBuffer = 0;

  uint64_t MAC0 = MACAddress0Port.Read() % 256;
  uint64_t MAC1 = MACAddress0Port.Read() / 256;
  uint64_t MAC2 = MACAddress2Port.Read() % 256;
  uint64_t MAC3 = MACAddress2Port.Read() / 256;
  uint64_t MAC4 = MACAddress4Port.Read() % 256;
  uint64_t MAC5 = MACAddress4Port.Read() / 256;

  // MAC address is big-endian encode so we just reverse it
  uint64_t MAC = MAC5 << 40
    | MAC4 << 32
    | MAC3 << 24
    | MAC2 << 16
    | MAC1 << 8
    | MAC0;

  // 32 bit mode
  registerAddressPort.Write(20);
  busControlRegisterDataPort.Write(0x102);

  // STOP reset
  registerAddressPort.Write(0);
  registerDataPort.Write(0x04);

  // initBlock
  initBlock.mode = 0x0000; // promiscuous mode = false
  initBlock.reserved1 = 0;
  initBlock.numSendBuffers = 3;
  initBlock.reserved2 = 0;
  initBlock.numRecvBuffers = 3;
  initBlock.physicalAddress = MAC;
  initBlock.reserved3 = 0;
  initBlock.logicalAddress = 0;

  sendBufferDescr = (BufferDescriptor*)((((uint32_t)&sendBufferDescMemory[0]) + 15) & ~((uint32_t)0xF));
  initBlock.sendBufferDescrAddress = (uint32_t)sendBufferDescr;
  recvBufferDescr = (BufferDescriptor*)((((uint32_t)&recvBufferDescMemory[0]) + 15) & ~((uint32_t)0xF));
  initBlock.recvBufferDescrAddress = (uint32_t)recvBufferDescr;
  
  for (uint8_t i = 0; i < 8; i++) {
    sendBufferDescr[i].address = (((uint32_t)&sendBuffers[i]) + 15) & ~(uint32_t)0xF;
    sendBufferDescr[i].flags = 0x7FF | 0xF000;
    sendBufferDescr[i].flags2 = 0;
    sendBufferDescr[i].avail = 0;

    recvBufferDescr[i].address = (((uint32_t)&recvBuffers[i]) + 15) & ~(uint32_t)0xF;
    recvBufferDescr[i].flags = 0x7FF | 0x80000000;
    recvBufferDescr[i].flags2 = 0;
    recvBufferDescr[i].avail = 0;
  }

  registerAddressPort.Write(1);
  registerDataPort.Write((uint32_t)(&initBlock) & 0xFFFF);

  registerAddressPort.Write(2);
  registerDataPort.Write(((uint32_t)(&initBlock) >> 16) & 0xFFFF);
}

amd_am79c973::~amd_am79c973() {

}

void amd_am79c973::Activate() {
  registerAddressPort.Write(0);
  registerDataPort.Write(0x41);

  registerAddressPort.Write(4);
  uint32_t temp = registerDataPort.Read();
  registerAddressPort.Write(4);
  registerDataPort.Write(temp | 0xC00);

  registerAddressPort.Write(0);
  registerDataPort.Write(0x42);
}

int amd_am79c973::Reset() {
  resetPort.Read();
  resetPort.Write(0);
  return 10;
}

uint32_t amd_am79c973::HandleInterrupt(uint32_t esp) {
  printf("INTERRUPT FROM AMD am79c973\n");

  registerAddressPort.Write(0);
  uint32_t temp = registerDataPort.Read();

  if ((temp & 0x8000) == 0x8000) printf("AMD am79c973 ERROR\n"); // general error
  if ((temp & 0x2000) == 0x2000) printf("AMD am79c973 COLLISION ERROR\n"); // collision error
  if ((temp & 0x1000) == 0x1000) printf("AMD am79c973 MISSED FRAME\n"); // missed frame
  if ((temp & 0x0800) == 0x0800) printf("AMD am79c973 MEMORY ERROR\n"); // memory error
  if ((temp & 0x0400) == 0x0400) Receive(); // data receved
  if ((temp & 0x0200) == 0x0200) printf("AMD am79c973 DATA SENT\n"); // data sent

  // acknowledge
  registerAddressPort.Write(0);
  registerDataPort.Write(temp);

  if ((temp & 0x0100) == 0x0100) printf("AMD am79c973 INIT DONE\n"); // init done

  return esp;
}

void amd_am79c973::Send(uint8_t* buffer, int size) {
  // get the number of currentSendBuffer
  int sendDescriptor = currentSendBuffer;

  // remove the currentSendBuffer cyclic to the next send buffer
  // that we could write or send data from different tasks in parallel
  currentSendBuffer = (currentSendBuffer + 1) % 8;

  // send more than 1518 bytes at once (this is too large)
  // then we'll just discard everything after that
  // we shouldn't be trying to send more at once
  // if we get a size larger thant that at this point made a mistake already
  if (size > 1518) {
    size = 1518;
  }

  // copy the data into the currentSendBuffer that we have selected here
  // so we set the `src` pointer to the end of the data that we want to send
  // and another `dst` pinter to the buffer where we want to write it
  // move these two pointer to the end of the buffers
  // `src` is the end of the `buffer`
  // `dst` is the end of the `sendBufferDescr`
  for (uint8_t *src = buffer + size - 1,
      *dst = (uint8_t*)(sendBufferDescr[sendDescriptor].address + size - 1);
      src >= buffer; src--, dst--) {
    // copy the data from the `src` buffer to the `dst` buffer
    *dst = *src;
  }

  // mark this buffer as in use
  // it's not allowed to write anything else in there until it's it becomes available again
  // with this we clear any error messages that have been there before
  sendBufferDescr[sendDescriptor].avail = 0;
  sendBufferDescr[sendDescriptor].flags2 = 0;

  // encode the size of what we wnat to send and
  // this here is the part that had this mistake in it
  //
  // See AMD am79c973 chip documentation page 186-188
  // https://www.amd.com/system/files/TechDocs/20550.pdf
  sendBufferDescr[sendDescriptor].flags = 0x8300F000
    | ((uint16_t)((-size) & 0xFFF));

  // this is a send command
  // write the command to send the data into the zeroes register
  registerAddressPort.Write(0);
  registerDataPort.Write(0x48);


}

void amd_am79c973::Receive() {
  printf("AMD am79c973 DATA RECEIVED\n");

  // iterate through the receive buffers as long as we have received buffers that contain data
  // in this loop, we move the currentRecvBuffer cyclic around
  // until we find a received buffer that has no data
  for (; (recvBufferDescr[currentRecvBuffer].flags & 0x80000000) == 0; currentRecvBuffer = (currentRecvBuffer + 1) % 8) {
    // receive buffer that hold data
    //
    // The first line checks the Error Bit (ERR)
    // The second line checks the Start-of-Packet(STP) and End-of-Packet(ENP) Bits
    //
    // See AMD am79c973 chip documentation page 184
    // https://www.amd.com/system/files/TechDocs/20550.pdf
    if (!(recvBufferDescr[currentRecvBuffer].flags & 0x40000000)
        && (recvBufferDescr[currentRecvBuffer].flags & 0x03000000) == 0x03000000)
    {
      // read the size from the recvBufferDescr
      uint32_t size = recvBufferDescr[currentRecvBuffer].flags & 0xFFF;

      // if the size is larger than 64, which this is the size of an Ethernet II frame
      // then remove the last four bytes
      // because they are a checksum and we are not really interested in the tricks on at this point 
      if (size > 64) { // remove checksum
        size -= 4;
      }

      uint8_t* buffer = (uint8_t*)(recvBufferDescr[currentRecvBuffer].address);

      if (handler != 0) {
        // if we receive something then we just sent this out again
        if (handler->OnRawDataReceived(buffer, size)) {
          Send(buffer, size);
        }
      }

      // now copy the address of the buffer and basically iterate over the data
      // print what we have received
      // uint8_t* buffer = (uint8_t*)(recvBufferDescr[currentRecvBuffer].address);
      // for (int i = 0; i < size; i++) {
      //   printfHex(buffer[i]);
      //   printf(" ");
      // }

      // in the end of the loop, we have finished handling this
      // so you can have this back
      recvBufferDescr[currentRecvBuffer].flags2 = 0;
      recvBufferDescr[currentRecvBuffer].flags = 0x8000F7FF;
    }
  };
}

void amd_am79c973::SetHandler(RawDataHandler* handler) {
  this->handler = handler;
}

// we need a way to ask father MAC address
uint64_t amd_am79c973::GetMACAddress() {
  return initBlock.physicalAddress;
}

