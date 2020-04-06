#ifndef __MYOS__NET__IPV4_H
#define __MYOS__NET__IPV4_H

#include <common/types.h>
#include <net/etherframe.h>
#include <net/arp.h>

namespace myos {

  namespace net {

    // IPv4 data block
    struct InternetProtocolV4Message {
      // put the length of header here, just for big endian encoding
      common::uint8_t headerLength : 4;
      common::uint8_t version : 4; // alway be 4, because of using IPv4
      common::uint8_t tos; // type of service
      common::uint16_t totalLength;

      common::uint16_t ident; // Identification
      common::uint16_t flagsAndOffset;

      common::uint8_t timeToLive;
      common::uint8_t protocol;
      common::uint16_t checksum;

      common::uint32_t srcIP;
      common::uint32_t dstIP;
    } __attribute__((packed));


    // we are doing the same system as etherframe
    class InternetProtocolProvider;

    class InternetProtocolHandler
    {
      protected:
        InternetProtocolProvider* backend;
        common::uint8_t ip_protocol; // one byte protocol for IPv4

      public:
        InternetProtocolHandler(InternetProtocolProvider* backend, common::uint8_t protocol);
        ~InternetProtocolHandler();

        // can have multiple connection to different IP on the same port
        // srcIP_BE and dstIP_BE used to distinguish different IP on the same port
        virtual bool OnInternetProtocolReceived(
            common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
            common::uint8_t* internetprotocolPayload, common::uint32_t size);
        void Send(common::uint32_t dstIP_BE, common::uint8_t* internetprotocolPayload, common::uint32_t size);
    };

    // we need EtherFrameHandler because it connect to EtherFrameProvider
    class InternetProtocolProvider : EtherFrameHandler
    {
      friend class InternetProtocolHandler;
      protected:
        InternetProtocolHandler* handlers[255];
        AddressResolutionProtocol* arp;
        common::uint32_t gatewayIP;
        common::uint32_t subnetMask;

      public:
        // arp for get MAC address
        InternetProtocolProvider(EtherFrameProvider* backend, 
            AddressResolutionProtocol* arp,
            common::uint32_t gatewayIP, common::uint32_t subnetMask);
        ~InternetProtocolProvider();

        bool OnEtherFrameReceived(common::uint8_t* etherframePayload, common::uint32_t size);

        void Send(common::uint32_t dstIP_BE, common::uint8_t protocol, common::uint8_t* buffer, common::uint32_t size);

        // The arp protocol allow you to set the check sum to zero. If checksum is zero,
        // it do not check the checksum. But IPv4 is more strict, if you don't have
        // correct checksum, then the receiver will be supposed to drop the message.
        static common::uint16_t Checksum(common::uint16_t* data, common::uint32_t lengthInBytes);
    };
  }
}

#endif

