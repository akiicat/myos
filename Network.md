# Network

## Ethernet Frame

// https://en.wikipedia.org/wiki/Ethernet
// https://en.wikipedia.org/wiki/Ethernet_frame

you can send and receive data now and that is basically like
being able like knowing the alphabet. Just because you know
the alphabet you cannot talk to someone. Because you don't
know his language possibly so in order to talk to someone,
you don't only need to know the alphabet but you also need
to know its language. The language in that metaphor
corresponds to the protocols. These protocals are really the
most common protocols. You need to support at least these
protocols to be able to communicate with the network. You
need to implement the Ethernet to frame protocol.

Ethernet II frame is really simple. It basically has the MAC
address of the source computer, the MAC address of the
destination computer, a protocol number, and some data.

Depending on the protocol number, you pass the Either to
the ARP address resolution protocol.

     12byte           2byte      
+--------------+-----------------+--------------------------------------------------------+
|      MAC     | Protocol number |                                                        |
+--------------+-----------------+--------------------------------------------------------+
                 0x0806 or 0x0800

Ethernet II frame
  + 0x0806 - Address Resolution Protocol (ARP)
  + 0x0800 - Internet Protocol (IPv4)

The source and destination MAC address are these first
12 bytes. Starting at the 12th byte, you have a 16-bit
integer which is a protocol number. If this protocol
number is 0x0806 it has to go to the address resolution
protocol (ARP) and if it is 0x0800 it's going to the Internet
Protocol (IP).

The Internet Protocol has its own header again which
contains for example the IP address of the source machine
and the destination machine.

The Ethernet to frame is really low-level. It's the
communication between the network chips. But the IP is a
logical address. If you didn't have a logical address, you
would have to reconfigure network every time you change out
a network device, and that would be really a lot of trouble
to go through and wouldn't really make sense.

In the header of the IP, you look at the offset 10. The first
10 bytes are the source IP, destination IP, and so on. By the
way, the address resolution protocol is there to ask a computer
"What is your IP address?". So if you don't implement ARP, your
machine will not communicate with anything, because the other
machine will try to ask you what is your IP address. And if you
haven't implemented ARP when you aren't responding and it will
just ignore you. So you need to implement ARP and the Internet
Protocol (IP) here at offset 10.

     12byte             2byte             10byte             1byte
+--------------+---------------------+--------------+--------------------+-------------------+-------------------------------------+
|  MAC Header  | MAC Protocol number |  IP Header   | IP Protocol number |        ...        | payload                             |
+--------------+---------------------+--------------+--------------------+-------------------+-------------------------------------+
                 0x0806 or 0x0800                   0x01 or 0x06 or 0x17

Ethernet II frame
  + 0x0806 - Address Resolution Protocol (ARP)
  + 0x0800 - Internet Protocol version 4 (IPv4)
      + 0x01 - Internet Control Message Protocol (ICMP)
      + 0x06 - Transmission Control Protocol (TCP)
      + 0x17 - User Datagram Protocol (UDP)
  + 0x8600 - Internet Protocol version 6 (IPv6)

You also have an 8-bit integer and depending on that value. If
this protocol number is 1 you need to pass it to the internet
control message protocol (ICMP). This is for example used for
`ping`, so you should really respond to ping. Because otherwise
computers will again think your machine is down and stop talking
to you.

If you have the protocol number 6, this is going to the
transmission control protocol (TCP). Maybe here you see
something TCP/IP now. That's where that name comes from it's the
combination of two protocols.

If protocol number is 17, it's going to the User Datagram
Protocol (UDP). You don't need to implement UDP but it's quite
simple. It's probably a better idea to implement UDP before TCP
because TCP is really really hard

This is really the architecture of the network stack, the OSI
layer model.


+-------------------------------------------------+
|                             +---------------+   |
|  Host Machine & Gateway     | Guest Machine |   |
|                             |               |   |
|  IP 10.0.2.2                | IP 10.0.2.15  |   |
|                             +---------------+   |
+-------------------------------------------------+


VirtualBox has a virtual IP address 10.0.2.2. This is the
default gateway. So when you send data where, it can go outside.
And actually this is also a virtual IP address with which you
can address your host machine. You have the host machine which
executes VirtualBox and in the VirtualBox you have the guest
machine. This is virtual machine terminology. So through 10.0.2.2
this IP address, you can communicate with the host machine.
If you run some server on the host machine and you have network
access in the guest machine, then you can access the server on
the host machine through the 10.0.2.2 IP address. And it's also
the default gateway. And you would assign something like the
IP address 10.0.2.15 or something to your guest machine. You
can assign Guest IP anything you want but it should really
have the same first bytes because of the subnet mask.

So let's talk a little bit about a subnet mask. Today you have
DHCP which assigns your in IP address and everything automatically,
but at least and previous years you had to assign your IP address
default gateway and subnet mask manually. What that means is the
subnet mask is something like 255.255.255.0. If when you want to
send some data out, you really want to know where do I send this.

    IP         10.  0.  2.  2
    Mask      255.255.255.  0
    Bitwise    10.  0.  2.  0

Inside a network maybe you have a switch and several machines
and they have relatively similar IP addresses which maybe only
differ in the last in the last byte. If they also
differ in the second to last byte you wouldn't meet 255.255.255.0
but 255.255.0.0. The subnet mask basically says if your IP
address bitwise and the subnet mask is equal to the target IP
address biwise and subnet mask, then you should think your system
is inside my own network, so I can communicate with that directly
through the switch. So I can just use that as my target IP address
and target MAC address. Otherwise, if the target is the same as a
source except for the bits that are zero in the subnet mask, then
we think it's the same network, and then we send our data directly
there through the switch. Otherwise, we sent it to the gateway
instead. Gateway take this you take care of it. It's a gateways
problem to send this out through the internet or something.

## Address Resolution Protocl (ARP)

https://en.wikipedia.org/wiki/Address_Resolution_Protocol

An ARP data block looks like this.

                                   2 bytes
   +---------------------------------------------------------------------+
 0 |                         Hardware Type (HTYPE)                       |
   +---------------------------------------------------------------------+
 2 |                         Protocol Type (PTYPE)                       |
   +----------------------------------+----------------------------------+
 4 |  Hardware (MAC) address length   |  Protocol (IP) address length    |
   +----------------------------------+----------------------------------+
 6 |                       Command / Operation (OPER)                    |
   +---------------------------------------------------------------------+
 8 |                                                                     |
   |                                                                     |
10 |                    Sender hardware (MAC) address (SHA)              |
   |                                                                     |
12 |                                                                     |
   +---------------------------------------------------------------------+
14 |                                                                     |
   |                    Sender protocol (IP) address (SPA)               |
16 |                                                                     |
   +---------------------------------------------------------------------+
18 |                                                                     |
   |                                                                     |
20 |                    Target hardware (MAC) address (THA)              |
   |                                                                     |
22 |                                                                     |
   +---------------------------------------------------------------------+
24 |                                                                     |
   |                    Target protocol (IP) address (TPA)               |
26 |                                                                     |
   +---------------------------------------------------------------------+


hardware type = 0x0100
protocol type = 0x0008
hardware address length = 6
protocol address length = 4
command


You get two bytes which tell you the **hardware type**. It tells you
this is going through Ethernet or is going to wireless LAN or something.
In our case this will always be one for Ethernet. This is two bytes
value and it's big-endian encoded so this will actually be 0x0100 the
big-endian enoding of 2 by 1.

Then we have 2 bytes for the **protocol type**. I mean we already have
an information about protocol is **ether type**, which will be 0x0806,
but the protocol that we have **ether type** is not he same as
**protocol type**. **Protocol type** tells us we are talking about IPv4
so this will be 0x0800. The ARP message talks about IPv4 which has code
0x0800 and again this is big-endian encoded so actually you would have
0x0008.

Then you have 2 fields of size 1 byte, **hardware address length** and 
**protocol address length**. **Hardware addredd length**, which tell
you the size of hardware address so this would be 6 because MAC
addresses have size 6, and here **protocol address length** we have 4
because IPv4 address have size of 4 bytes.

Then we have 2 more bytes with a **command**, which tells us what does
the sender of this message want from us. It asks us what is your IP
address or is it a response to our quenstion. Maybe we have asked him
what is your IP address and this is the response. These are actually
the two commands that we will implement.

After that we have 6, 4, 6, 4 bytes which contain **MAC address of sender**,
**IP address of sender**, **MAC address of receiver**, **IP address of receiver**.
If the command tell me your IP address, then IP address of receiver
won't be a legal value or maybe if we get it from a abroadcast then 
the MAC address of receiver will be a broadcast address.

When we send something back, we will copy the MAC address of sender
block to the MAC address of receiver block because we are sending
this back to the sender of the message. But actually not all ARP
messages have this exact length. The lengths of the stuff after
the **command** can differ based on these two values, hardware
address length and protocol address length. But we will really hard
code this to be 6 and 4 bytes.

The ARP class needs to know the IP address of the machine because
how do you know what you answers a question for your IP address if
you don't know your IP address, so it needs to get the IP address
somehow. But ARP is in the class for the network card, the network
card is init block and had 4 bytes for an IP address. 

amd_am79c973
  + Ethernet II frame
    + 0x0806 - Address Resolution Protocol (ARP)
    + 0x0800 - Internet Protocol version 4 (IPv4)
        + 0x01 - Internet Control Message Protocol (ICMP)
        + 0x06 - Transmission Control Protocol (TCP)
        + 0x17 - User Datagram Protocol (UDP)
    + 0x8600 - Internet Protocol version 6 (IPv6)

We will tell amd_am79c973 this class this is your IP address, and
ARP class is supposed to ask Ethernet II class, and Ethernet II
class is supposed to ask amd_am79c973 class.



By the way, if you ask yourself why do we have the MAC address
here again (both Ethernet and ARP have MAC addresses).
So you might think why we have MAC addresses here again, you
could just look at these instead. While you cannot say I will
just leave MAC addresses out in ARP and just use MAC addresses
in EtherFrame instead, because there can be multiple machines
between the sender and the destination.

  Machine A  --->  Machine B  --->  Machine C

When Machine A wants to talk to Machine C, Machine A MAC and
IP addresses will be ARP packet srcMAC and srcIP, and Machine
C MAC and IP addressess will be ARP packet dstMAC and srcIP.
But Machine A will have to send the whole to Machine B first,
when Machine A communicates with Machine B, the Ether source
MAC address will be Machine A, and the Ether destination MAC
address will be Machine B not Machine C.

