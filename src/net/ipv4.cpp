#include <net/ipv4.h>

using namespace myos;
using namespace myos::common;
using namespace myos::net;

InternetProtocolHandler::InternetProtocolHandler(InternetProtocolProvider* backend, uint8_t protocol) {
  this->backend = backend;
  this->ip_protocol = protocol;
  backend->handlers[protocol] = this;
}

InternetProtocolHandler::~InternetProtocolHandler() {
  if (backend->handlers[ip_protocol] == this) {
    backend->handlers[ip_protocol] = 0;
  }
}

bool InternetProtocolHandler::OnInternetProtocolReceived(
    uint32_t srcIP_BE, uint32_t dstIP_BE,
    uint8_t* internetprotocolPayload, uint32_t size) {
  // not do anything on received, reserved for ICMP, TCP, UDP
  return false;
}

void InternetProtocolHandler::Send(uint32_t dstIP_BE, uint8_t* internetprotocolPayload, uint32_t size) {
  // pass to the backend
  backend->Send(dstIP_BE, ip_protocol, internetprotocolPayload, size);
}

InternetProtocolProvider::InternetProtocolProvider(EtherFrameProvider* backend, 
    AddressResolutionProtocol* arp,
    uint32_t gatewayIP, uint32_t subnetMask)
: EtherFrameHandler(backend, 0x800) { // 0x800 for IP
  for (int i = 0; i < 255; i++) {
    handlers[i] = 0;
  }
  this->arp = arp;
  this->gatewayIP = gatewayIP;
  this->subnetMask = subnetMask;
}

InternetProtocolProvider::~InternetProtocolProvider() {
}

bool InternetProtocolProvider::OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size) {
  if (size < sizeof(InternetProtocolV4Message)) {
    return false;
  }

  InternetProtocolV4Message* ipmessage = (InternetProtocolV4Message*)etherframePayload;
  bool sendBack = false;

  // check the message is for us
  if (ipmessage->dstIP == backend->GetIPAddress()) {
    // check which is the minimal length between total data length and size
    int length = ipmessage->totalLength;
    if (length > size) {
      length = size;
    }

    // look the handler for the protocol and call IPv4 received method
    // and retrue if need to send data back
    if (handlers[ipmessage->protocol] != 0) {
      sendBack = handlers[ipmessage->protocol]->OnInternetProtocolReceived(
          ipmessage->srcIP, ipmessage->dstIP, 
          etherframePayload + 4*ipmessage->headerLength, length - 4*ipmessage->headerLength);
    }
  }

  if(sendBack) {
    // switch the srcIP and dstIP
    uint32_t temp = ipmessage->dstIP;
    ipmessage->dstIP = ipmessage->srcIP;
    ipmessage->srcIP = temp;

    ipmessage->timeToLive = 0x40; // set TTL to 64

    // We change the header, so we need to update checksum.
    ipmessage->checksum = 0;
    ipmessage->checksum = Checksum((uint16_t*)ipmessage, 4*ipmessage->headerLength);
  }

  return sendBack;
}

void InternetProtocolProvider::Send(uint32_t dstIP_BE, uint8_t protocol, uint8_t* data, uint32_t size) {
  uint8_t* buffer = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(InternetProtocolV4Message) + size);
  InternetProtocolV4Message *message = (InternetProtocolV4Message*)buffer;

  message->version = 4; // version 4 for ipv4
  message->headerLength = sizeof(InternetProtocolV4Message) / 4;
  message->tos = 0; // 0 for not privileged message

  // set the total of length. Switch byte because of big endian value.
  message->totalLength = size + sizeof(InternetProtocolV4Message);
  message->totalLength = ((message->totalLength & 0xFF00) >> 8)
                       | ((message->totalLength & 0x00FF) << 8);

  message->ident = 0x0100; // set Identification to a random number 1
  message->flagsAndOffset = 0x0040;
  message->timeToLive = 0x40; // 64
  message->protocol = protocol;

  message->dstIP = dstIP_BE;
  message->srcIP = backend->GetIPAddress();

  // We need to initialize checksum to zero, because the checksum also add
  // this value. Checksum will be invalid and packet will be dropped.
  message->checksum = 0;
  message->checksum = Checksum((uint16_t*)message, sizeof(InternetProtocolV4Message));

  // copy the message we suppose to send
  uint8_t* databuffer = buffer + sizeof(InternetProtocolV4Message);
  for (int i = 0; i < size; i++) {
    databuffer[i] = data[i];
  }

  // By default, we want to send the data to destination.
  // But if the target is not in our subnet, is outside the local network,
  // just send the message to the gateway.
  uint32_t route = dstIP_BE;
  if ((dstIP_BE & subnetMask) != (message->srcIP & subnetMask)) {
    route = gatewayIP;
  }

  backend->Send(arp->Resolve(route), this->etherType_BE, buffer, sizeof(InternetProtocolV4Message) + size);

  MemoryManager::activeMemoryManager->free(buffer);
}

uint16_t InternetProtocolProvider::Checksum(uint16_t* data, uint32_t lengthInBytes) {
  uint32_t temp = 0;

  // Add the data as big endian
  for (int i = 0; i < lengthInBytes / 2; i++) {
    temp += ((data[i] & 0xFF00) >> 8) | ((data[i] & 0x00FF) << 8);
  }

  // If data is odd number, there will be one number at the end
  if (lengthInBytes % 2) {
    temp += ((uint16_t)((char*)data)[lengthInBytes-1]) << 8;
  }

  // If temp is overflow, then back to 16 bits until first 16 bits are zeros
  while (temp & 0xFFFF0000) {
    temp = (temp & 0xFFFF) + (temp >> 16);
  }

  // temp must be bitwise
  // return this value to big endian
  return ((~temp & 0xFF00) >> 8) | ((~temp & 0x00FF) << 8);
}

