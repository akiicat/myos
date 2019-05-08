#ifndef __MYOS__DRIVERS__VGA_H
#define __MYOS__DRIVERS__VGA_H

#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/port.h>

namespace myos {

  namespace drivers {

    class VideoGraphicsArray {
      protected:
        hardwarecommunication::Port8Bit miscPort; // miscellaneous stuff
        hardwarecommunication::Port8Bit crtcIndexPort; // cathode ray tube controller index
        hardwarecommunication::Port8Bit crtcDataPort; // cathode ray tube controller data 
        hardwarecommunication::Port8Bit sequencerIndexPort;
        hardwarecommunication::Port8Bit sequencerDataPort;
        hardwarecommunication::Port8Bit graphicsControllerIndexPort;
        hardwarecommunication::Port8Bit graphicsControllerDataPort;
        hardwarecommunication::Port8Bit attributeControllerIndexPort;
        hardwarecommunication::Port8Bit attributeControllerReadPort;
        hardwarecommunication::Port8Bit attributeControllerWritePort;
        hardwarecommunication::Port8Bit attributeControllerResetPort;

        // sends these initialization codes to the coresponding ports
        void WriteRegisters(common::uint8_t* registers);

        // we will have a method which gives us the correct offset
        // for the segment that we want to use
        common::uint8_t* GetFrameBufferSegment();

        // the PutPixel method actually needs to get this color index for a certain color
        virtual common::uint8_t GetColorIndex(common::uint8_t r, common::uint8_t g, common::uint8_t b);

      public:
        VideoGraphicsArray();
        ~VideoGraphicsArray();

        // a method to set the mode
        virtual bool SetMode(common::uint32_t width, common::uint32_t height, common::uint32_t colordepth);

        // tell us if a certain mode is even supported
        virtual bool SupportsMode(common::uint32_t width, common::uint32_t height, common::uint32_t colordepth);

        // be used to GetFrameBufferSegment
        // a method PutPixel which accepts a 24-bit color-code red green blue
        // this method will put this color (r, g, b) to the coordinate (x, y)
        //
        // the PutPixel will call GetColorIndex to get the same index for this color
        // we could give it to many different colors,
        // so the drivers are supposed to hide away the fact that
        // it cannot actually represent this many colors
        // 
        // so we might have only one light blue and a different light blue
        // they might get drawn as the same light blue in the end
        // this is supposed to be hidden from all those other things that draw on the screen
        //
        // the object that draw on the screen are not supposed to know that you have only 8-bit color depth in behind this
        virtual void PutPixel(common::uint32_t x, common::uint32_t y, common::uint8_t r, common::uint8_t g, common::uint8_t b);

        // vga will be using an 8-bit version for now
        // so we cannot actually set a color like 24-bit version
        //
        // select an index in the table of 8-bit table (256 entries)
        //
        // make this method public for now
        // shouldn't be in the long run
        void PutPixel(common::uint32_t x, common::uint32_t y, common::uint8_t colorIndex);


    };




  }
}

#endif
