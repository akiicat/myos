#include <net/etherframe.h>

using namespace myos;
using namespace myos::common;
using namespace myos::net;
using namespace myos::drivers;

EtherFrameHandler::EtherFrameHandler(EtherFrameProvider* backend, common::uint16_t etherType) {
  // convert ethertype to big endian
  this->etherType_BE = ((etherType & 0x00FF) << 8)
                     | ((etherType & 0xFF00) >> 8);
  this->backend = backend;
  backend->handlers[etherType_BE] = this;
}

EtherFrameHandler::~EtherFrameHandler() {
  backend->handlers[etherType_BE] = 0;
}

bool EtherFrameHandler::OnEtherFrameReceived(common::uint8_t* etherframePayload, common::uint32_t size) {
  // just return false, because we are not sending anything like that
  return false;
}

void EtherFrameHandler::Send(common::uint64_t dstMAC_BE, common::uint8_t* data, common::uint32_t size) {
  backend->Send(dstMAC_BE, etherType_BE, data, size);
}

EtherFrameProvider::EtherFrameProvider(amd_am79c973* backend)
  : RawDataHandler(backend)
{
  for (uint32_t i = 0; i < 65535; i++) {
    handlers[i] = 0;
  }
}
  
EtherFrameProvider::~EtherFrameProvider() {

}

bool EtherFrameProvider::OnRawDataReceived(common::uint8_t* buffer, common::uint32_t size) {
  // we would put EtherFrameHeader structure over it
  EtherFrameHeader* frame = (EtherFrameHeader*)buffer;
  bool sendBack = false;

  // if frame destinations is either a broadcast or it's really for us,
  // then we handle it, look into the handlers, and see if we have
  // handler for this EtherType
  if (frame->dstMAC_BE == 0xFFFFFFFFFFFF ||
      frame->dstMAC_BE == backend->GetMACAddress()) {
    if (handlers[frame->etherType_BE] != 0) {
      // we call a handler method but we wnat to give it only the data
      // inside the frame
      // pass the pointer to the buffer plus the size of header
      sendBack = handlers[frame->etherType_BE]->OnEtherFrameReceived(
          buffer + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
    }
  }

  // We have gotten the data. Now the handler does something on
  // it maybe for example the ARP handle. It's answer into this data
  // and returns true. Then what we have to do is we have to return
  // true also so that the driver sends this whole data back. But
  // we have to switch the source and the destination so that the
  // network understands that this is now going to the computers
  // that send us the request before. Now, we have another problem
  // the destination could be the broadcast address 0xFFFFFFFFFFFF.
  // What we will do is we will copy the source to the destination
  // and set the source to our MAC address.
  // And then everything is fine :)
  if (sendBack) {
    frame->dstMAC_BE = frame->srcMAC_BE;
    frame->srcMAC_BE = backend->GetMACAddress();
  }

  return sendBack;
}

void EtherFrameProvider::Send(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, common::uint8_t* src_buffer, common::uint32_t size) {

  // we get the memory from the header plus the size of the buffer that we want to send
  uint8_t* dst_buffer = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(EtherFrameHeader) + size);
  EtherFrameHeader* frame = (EtherFrameHeader*)src_buffer;

  // impose a header on it again
  frame->dstMAC_BE = dstMAC_BE;
  frame->srcMAC_BE = backend->GetMACAddress();
  frame->etherType_BE = etherType_BE;

  // we just copy the buffer from this `src_buffer` to this `dst_buffer`
  uint8_t* src = src_buffer;
  uint8_t* dst = dst_buffer + sizeof(EtherFrameHeader);
  for (uint32_t i = 0; i < size; i++) {
    dst[i] = src[i];
  }

  // pass `dst_buffer` the backend
  backend->Send(dst_buffer, size + sizeof(EtherFrameHeader));
}

uint32_t EtherFrameProvider::GetIPAddress() {
  return backend->GetIPAddress();
}

uint64_t EtherFrameProvider::GetMACAddress() {
  return backend->GetMACAddress();
}
