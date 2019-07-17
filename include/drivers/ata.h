#ifndef __MYOS__DRIVERS__ATA_H
#define __MYOS__DRIVERS__ATA_H

#include <hardwarecommunication/port.h>
#include <common/types.h>

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
        void Read28(common::uint32_t sector);
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
