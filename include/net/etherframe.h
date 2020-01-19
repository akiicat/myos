// We already have is a driver for this AMD PC net device AM79C973
// and we can send raw data with that and we can receive raw data.
// What I want to do today is I want to attach a handler for
// Ethernet Frame raw data. Ethernet Frame should look at the raw
// data from AM79C973 and decides to which of the next protocols
// it passes this data on.
// 
//                                       +->  0x0806 ARP     +->  0x01 ICMP
//                                       |                   |
// AM79C973  +--->  Ethernet Frame II  +--->  0x0800 IPv4  +--->  0x06 TCP
//                                       |                   |
//                                       +->  0x8600 IPv6    +->  0x17 UDP
// 
// Ethernet II frame
//   + 0x0806 - Address Resolution Protocol (ARP)
//   + 0x0800 - Internet Protocol version 4 (IPv4)
//       + 0x01 - Internet Control Message Protocol (ICMP)
//       + 0x06 - Transmission Control Protocol (TCP)
//       + 0x17 - User Datagram Protocol (UDP)
//   + 0x8600 - Internet Protocol version 6 (IPv6)
// 
// https://en.wikipedia.org/wiki/Ethernet_frame
// 
// The Wikipedia page on the topic is really good. You can see
// here how the raw data is structured, so you get raw data and
// the first six bytes are the MAC address of the destination.
// 
// So the next six bytes should be our MAC address if it's
// directed at us or it can be 0xFFFFFFFFFFFF that is a
// broadcast which is sent to all the participants in a
// network.
// 
//            6bytes                    6bytes             2bytes        46 to 1500 bytes           4bytes
// +-------------------------+-------------------------+-----------+---------------------------+--------------+
// | Destination MAC Address |    Source MAC Address   | EtherType |  Payload (IP, ARP, etc.)  | CRC Checksum |
// +-------------------------+-------------------------+-----------+---------------------------+--------------+
// |                     MAC Header (14bytes)                      |  Data (46 to 1500bytes)   |              |
// +---------------------------------------------------------------+---------------------------+--------------+
// |                            Ethernet Type II Frame (64 to 1518bytes)                                      |
// +----------------------------------------------------------------------------------------------------------+
// 
// For example when you will turn on a computer it sends a
// broadcast and tells everyone "hey I'm here who else is
// here." So you have 6 bytes for the source who sent this
// data. Then you have two bytes with other so-called EtherType
// and then you have a Checksum four bytes. and in between
// EtherType and Checksum, there is the data.
// 
// so we could look at the MAC destination and see this is for
// us and if so then we could look at the EtherType. So if it's
// 0x0806, then we give it to the ARP handler. If it's 0x0800,
// then we give it to the ipv4 handler and so on.
// 
// But I want to do this again in a more object-oriented way
// where I will have an array of these handlers and then such as
// handler can write itself into this array.
// 
// So that we have this inversion of control paradigm again
// which is very common in the object-oriented programming.
// We will get the data from the trip and we just have to look
// at it and decide whom do we give this to.

#ifndef __MYOS__NET__ETHERFRAME_H
#define __MYOS__NET__ETHERFRAME_H

#include <common/types.h>
#include <drivers/amd_am79c973.h>
#include <memorymanagement.h>

namespace myos {

  namespace net {

    // make a ether frame structure (see above description)
    struct EtherFrameHeader {
      common::uint64_t dstMAC_BE : 48; // mac addresses hava only six bytes
      common::uint64_t srcMAC_BE : 48;
      common::uint16_t etherType_BE;
    } __attribute__((packed));

    typedef common::uint32_t EtherFrameFooter;

    class EtherFrameProvider;

    // A Single Ether Frame, for example:
    //   etherType_BE  Name
    //   0x0806        ARP  
    //   0x0800        IPv4 
    //   0x8600        IPv6 
    class EtherFrameHandler {
      protected:
        EtherFrameProvider* backend;
        common::uint16_t etherType_BE;

      public:
        EtherFrameHandler(EtherFrameProvider* backend, common::uint16_t etherType);
        ~EtherFrameHandler();

        virtual bool OnEtherFrameReceived(common::uint8_t* etherframePayload, common::uint32_t size);
        void Send(common::uint64_t dstMAC_BE, common::uint8_t* etherframePayload, common::uint32_t size);

    };

    // A group of EtherFrameHandler
    class EtherFrameProvider : public drivers::RawDataHandler {
      friend class EtherFrameHandler;

      protected:
        EtherFrameHandler* handlers[65535];

      public:
        EtherFrameProvider(drivers::amd_am79c973* backend);
        ~EtherFrameProvider();

        bool OnRawDataReceived(common::uint8_t* buffer, common::uint32_t size);
        void Send(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, common::uint8_t* buffer, common::uint32_t size);
    };
  }
}

#endif
