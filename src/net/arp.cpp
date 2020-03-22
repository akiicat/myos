#include <net/arp.h>

using namespace myos;
using namespace myos::common;
using namespace myos::net;
using namespace myos::drivers;

void printf(char*);
void printfHex(uint8_t);

AddressResolutionProtocol::AddressResolutionProtocol(EtherFrameProvider* backend)
  : EtherFrameHandler(backend, 0x806) // 0x806 for ARP
{
  numCacheEntries = 0;
}

AddressResolutionProtocol::~AddressResolutionProtocol() {

}

bool AddressResolutionProtocol::OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size) {
  printf("ARP RECV:\n");
  // When we reciev such a message, we cast it to the message type first.
  // But we should only do this if the size is large enough.
  if (size < sizeof(AddressResolutionProtocolMessage)) {
    return false;
  }

  AddressResolutionProtocolMessage* arp = (AddressResolutionProtocolMessage*)etherframePayload;

  // If message is for us, we can answer it.
  // If message is not for us, then we might look into our cache
  // for the requested IP address and answer it.
  //
  // In this case, I'm not going to do anything. Otherwise, we
  // might want to pass the requests on to some other machine.
  // That would be the behavior of a gateway, and we are not making
  // a gateway right now.
  if (arp->hardwareType == 0x0100) {
    if (arp->protocol == 0x0008 &&
        arp->hardwareAddressSize == 6 &&
        arp->protocolAddressSize == 4 &&
        arp->dstIP == backend->GetIPAddress()
        ) {

      // If message is for us, look what is the command.
      switch(arp->command) {

        case 0x0100: // request
          // If we are asked for our mac address, we change the command
          // into a response.
          arp->command = 0x0200;

          // now we turn the sender to recipient
          arp->dstIP = arp->srcIP;
          arp->dstMAC = arp->srcMAC;

          // Source is going to be our IP address and our MAC address.
          arp->srcIP = backend->GetIPAddress();
          arp->srcMAC = backend->GetMACAddress();

          // Send it back in the end
          return true;
          break;

        case 0x0200: // response
          // We have gotten a response to a request that we have made.
          // Then we just insert the response into our cache.
          //
          // If we get a response for an IP address that we asked for
          // maybe it's not a good idea to store it somewhere. These
          // message are unreliable. So If you ask maybe for the IP
          // of google and someone answer with his own mac address,
          // then effectively you will be talking to him whenever you
          // make a Google request. And he behind that he might talk
          // to Google and answer your requests, but he might read what
          // you are doing and he might change the results. This is
          // really a point where a man-in-the-middle attack can
          // happen through something called ARP spoofing. This is just
          // unreliable, I don't think there's really something you
          // can do against it, because you are getting some data and
          // you just cannot validate that where it came from. Maybe,
          // if you do something like communication through SSL, then
          // you need the private key and publick. For now, we have
          // no other choice than to trust this message.
          if (numCacheEntries < 128) {
            IPcache[numCacheEntries] = arp->srcIP;
            MACcache[numCacheEntries] = arp->srcMAC;
            numCacheEntries++;
          }
          break;
      }

    }
  }

  return false;
}

void AddressResolutionProtocol::RequestMACAddress(uint32_t IP_BE) {
  // see Network.md
  AddressResolutionProtocolMessage arp;
  arp.hardwareType = 0x0100; // ethernet
  arp.protocol = 0x0008; // ipv4
  arp.hardwareAddressSize = 6; // mac
  arp.protocolAddressSize = 4; // ipv4
  arp.command = 0x0100; // request

  arp.srcMAC = backend->GetMACAddress();
  arp.srcIP = backend->GetIPAddress();

  // we want to talk to this machine, **IP_BE**, but we don't know
  // its mac address, so we send a request for that.
  arp.dstMAC = 0xFFFFFFFFFFFF; // broadcast
  arp.dstIP = IP_BE;

  // inherited from EtherFrameHandler::Send
  this->Send(arp.dstMAC, (uint8_t*)&arp, sizeof(AddressResolutionProtocolMessage));
}

uint64_t AddressResolutionProtocol::GetMACFromCache(uint32_t IP_BE) {
  // In reading something from the cache, we will just iterate through
  // the cacche and return if we find something otherwise we will
  // return 0
  for (int i = 0; i < numCacheEntries; i++) {
    if (IPcache[i] == IP_BE) {
      return MACcache[i];
    }
  }

  return 0xFFFFFFFFFFFF; // broadcast address
}

uint64_t AddressResolutionProtocol::Resolve(uint32_t IP_BE) {
  // First we look if we already have the mac addres in our cache.
  // If not then we request it.
  uint64_t result = GetMACFromCache(IP_BE);

  // 0xFFFFFFFFFFFF is what we get if the mac isn't in the cache.
  if (result == 0xFFFFFFFFFFFF) {
    RequestMACAddress(IP_BE);
  }

  // this isn't really safe because if the machine isn't even
  // connected, then of course you will never get answer to this
  // request. You will never get an entry in the cache, then
  // this loop will never terminate.
  //
  // So we should have some way of finding out some time out here
  // I'll leave it like this for now.
  while (result == 0xFFFFFFFFFFFF) { // possible infinite loop
    result = GetMACFromCache(IP_BE);
  }

  return result;
}
