#ifndef __MYOS__DRIVERS__ATA_H
#define __MYOS__DRIVERS__ATA_H

#include <hardwarecommunication/port.h>
#include <common/types.h>

// PCI settings for hard disk driver:
// - class ID: 0x01
// - subclass ID: 0x06 for SATA
//
// History of Hard Drive:
//
//   IDE hard dirve --> advance technology attachment (ATA) -->  Serial ATA (SATA)
//                   ^                                       ^
//         used the same as IDE                   commpatiable to ATA or AHCI (Advanced host controller interface)
//
// VirtualBox only offers AHCI when we include in SATA contoller
// in our virtual machine. And in VirtualBox, we will have to
// choose to add IDE controller not in SATA controller because if
// we take SATA only have AHCI but we want to do IDE or ATA.
//
// Now there are two different ways of accessing a hard drive: PIO and DMA
//
// PIO (Programe Input-Output) that is relatively slow but
// that is the one that we implemented because it's relatively simple.
// The processor basically talks to the controller all the time
// say "give me the next byte". That works but it's not fast
// around 16 MB/sec which is really not much.
//
// The better way would be DMA (Direct memory Access). In DMA you
// basically tell the controller "hey just write the data to this
// memory location and give an interrupt when you're finished".
// If you do it with DMA or Urtra DMA nowadays, you of course
// that gives you much better performance because while the
// controller is writing the data into the RAM, the processor is
// free to do other things so that gives you really much better
// performance, but it's also more complicated.
//
// PCI controllers are still relatively stadardize the port numbers,
// interrupt numbers for hard drive.
//
//      PIO
//     /   \
//  28bit 48bit
//
// The PIO mode has 28 bits and 48 bits mode. These numbers just
// say how many bits do you transfer to the device to request the
// sector. Now you don't request every byte indiviually. You
// request a bunch of bytes (512 bytes). 512 bytes is normally a
// sector.
//
// If you have 512 bytes multipled with the 28 bit then you get
// the 4 GB. You could not write files that larger than 4 GB.
// That is the reason there were times. When you couldn't have
// hard drive larger than 2 GB for the same reason. And you just
// had one of the bits were used more other thins, so they used
// signed integer, and you could only have 2 GB of hard drive.
//
// With 48 bits you can access up to 128 PB that's a bit more
// then 4 GB, but we will be doing 28 bits because they are
// basically the same things. They work very very similar.
//
// When you deal with hard drives, you will probably come across
// the 3 more terms:
// - cylinders
// - heads
// - sectors
// Back in the time of the IDE, you probably know that hard
// drives look like disc. They have several discs and several
// read and write heads. The operating system basically had to
// know the hard drives to take control. So the operating system 
// would say "I want to talk to the cylinder number, the head
// number, the sector number". But that is referred to
// CHS (Cylinder Head Sector) addressing. So to select a sector
// on a hard drive, you would have to transmit these three
// values.
//
// But in the mean time, between IDE and ATA, they came up with
// the better idea because it's not really good that you have
// to know the geometry of the hard drive to be able to talk to it.
// Nowadays, you have Soild-State Drive (SSD) and they don't even
// have cylinders, heads and sectors. So, it would totally not
// make sense to talk to a SSD in CHS addressing mode. What they
// came up with is the Logical Block Address (LBA). That basically
// just says "Here is a number I want to have the sector with that
// that number". After that, it's the hard drives problem to
// translate that LBA into CHS address. So it's not your problem
// as the developer of an operating system. You shouldn't have to
// deal with such technical details of our devices. That should
// offer you an interface like a black box, but there is actually
// a formula to call to translate between CHS addressing mode and
// LBA addressing mode.
// 
// You look at the program `fdisk` on Linux. When you partition a
// hard drive with that, the Master Boot Record (MBR) has a very
// space for CHS addresses but also for LBA addresses. `fdisk`
// writes both of them so it should be sufficient to only write
// LBA. LBA and CHS are just address bytes in the Master Boot Record (MBR)
// or in the BIOS Parameter Block (BPB)

namespace myos {

  namespace drivers {

    class AdvancedTechnologyAttachment {
      protected:
        // communicate the controller has 9 ports

        // the data port through which we sent the data that we want to write
        // and we pull the data that we read but
        hardwarecommunication::Port16Bit dataPort;

        // error messages for reading error messages
        hardwarecommunication::Port8Bit errorPort;

        // through this we tell the hard drive how many sectors we want to read
        hardwarecommunication::Port8Bit sectorCountPort;

        // three ports through which transmit the logical block address (LBA)
        // of the sector that we want to read
        hardwarecommunication::Port8Bit lbaLowPort;
        hardwarecommunication::Port8Bit lbaMidPort;
        hardwarecommunication::Port8Bit lbaHiPort;

        // through this port we basically say whether we want to talk to the master or the slave
        // and in 28 bit mode we also transfer part of absolute record block address of the sector
        // that we want to read throught that
        hardwarecommunication::Port8Bit devicePort;

        // command port which we transfer the instruction 
        // we say I want to read this or I want ot write to this part after the sector
        hardwarecommunication::Port8Bit commandPort;

        // control port which is also used for status messages
        hardwarecommunication::Port8Bit controlPort;

        // you can also read the information how many bytes are in a sector
        // but I'm not going to this. Just set this number to 512 that's it.
        bool master;
        common::uint16_t bytesPerSector;

      public:
        // hard code the port number
        // we do have multiple of the ATA buses
        // I cannot hard code into the ata.cpp
        // I have to pass it from the kernel later
        //
        // store a boolean which says if this is a handler for the master or the slave
        // which are two hard drives on the same bus
        // actually the words master and slave don't mean anything
        // it's just a random words to describe to distinguish one of the hard drives and the other one
        AdvancedTechnologyAttachment(common::uint16_t portBase, bool master);
        ~AdvancedTechnologyAttachment();

        // the operations that we really wnat to implement
        // the Identify operation through which we talked to the hardware
        // and ask are you there and what kind of hard drive are you install
        void Identify();

        // sector number is a 32 bit integer
        // so the highest bits of that just need to be ignored
        void Read28(common::uint32_t sector, common::uint8_t* data, int count);
        void Write28(common::uint32_t sector, common::uint8_t* data, int count);

        // to flush the cache of half drive because the thing is when you write to a hard drive,
        // the hard drive first buffers that data in an internal buffer and then you have to flush the buffer
        // so that the hard drive really writes the data to the disk and
        // if you don't do that and you pull the plug of your computer
        // so you have written something to a hard drive and not flushed it
        // then the sector is goint to be lost
        // sot the hard drive you will not be able to read from that the sector again
        // until you write something to that sector again and then flush again
        // then this sector is not lost anymore
        //
        // in the long term you have to do something like journaling where you are about to 
        // do somehing like two journals and at least one of thme always has to be must have integrity
        // so you would do something like if you want to write to some sector you would write
        // to the first journal "I want to write to that sector".
        // Now this journal is doesn't have integrity, so if you pull the plug now
        // and the first journal is broken but the second one is still intact
        //
        // if you flush the first journal, now I have two different journals which don't agree
        // and then you write the same information to the second journal and flush
        // Now if you pull the plug now then the first journal is still intack and the second one is broken.
        // But if the power doesn't go down and you flush again now, you have two intact journals again.
        // So you will always have an intact journal.
        // 
        // These journal entries basically say "I want to write to this sector now".
        // Once you have flushed the first sector and do the same thing on the second sector
        // and kick this information out of the first and second journals again
        // 
        // The benefit of this is if I write and do not flush because the power goes down,
        // then I reboot I can see the first sector must be broken.
        // Maybe I just couldn't update the journal after it was already update,
        // but I can directly identify which sectors are broken and recover them
        // If I don't do that after such a power failure, you would have to
        // scan the whole hard drive (chkdsk command for windows) and look for broken sectors
        // That is what you had to do regularly in the 1990s where you had to
        // run the program scan disk regularly. And often Windows would start scan disk on
        // boot process. That is the main reason for that.
        void Flush();


    };

  }

}

#endif
