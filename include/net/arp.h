#ifndef __MYOS__NET__ARP_H
#define __MYOS__NET__ARP_H

#include <common/types.h>
#include <net/etherframe.h>

namespace myos {

  namespace net {

    // Internet Protocol (IPv4) over Ethernet ARP packet
    // https://en.wikipedia.org/wiki/Address_Resolution_Protocol
    struct AddressResolutionProtocolMessage {
      common::uint16_t hardwareType;
      common::uint16_t protocol;
      common::uint8_t hardwareAddressSize; // 6
      common::uint8_t protocolAddressSize; // 4

      common::uint16_t command;

      common::uint64_t srcMAC : 48;
      common::uint32_t srcIP;
      common::uint64_t dstMAC : 48;
      common::uint32_t dstIP;
    } __attribute__((packed));

    class AddressResolutionProtocol : public EtherFrameHandler {
      private:

        common::uint32_t IPcache[128];
        common::uint64_t MACcache[128];
        int numCacheEntries;

      public:
        AddressResolutionProtocol(EtherFrameProvider* backend);
        ~AddressResolutionProtocol();

        // overwrite EtherFrameHandler function OnEtherFrameReceived
        bool OnEtherFrameReceived(common::uint8_t* etherframePayload, common::uint32_t size);

        // give IP address with big-endian encoding and we want it
        // to send a request for the MAC address of this IP address
        void RequestMACAddress(common::uint32_t IP_BE);

        // look for entry in cache
        common::uint64_t GetMACFromCache(common::uint32_t IP_BE);

        // a method that send the request waits for an answer and
        // then returns the MAC address.
        common::uint64_t Resolve(common::uint32_t IP_BE);
    };

  }

}

#endif
