#include <drivers/vga.h>

using namespace myos::common;
using namespace myos::drivers;

VideoGraphicsArray::VideoGraphicsArray() :
  miscPort(0x3C2),
  crtcIndexPort(0x3D4),
  crtcDataPort(0x3D5),
  sequencerIndexPort(0x3C4),
  sequencerDataPort(0x3C5),
  graphicsControllerIndexPort(0x3CE),
  graphicsControllerDataPort(0x3CF),
  attributeControllerIndexPort(0x3C0),
  attributeControllerReadPort(0x3C1),
  attributeControllerWritePort(0x3C0),
  attributeControllerResetPort(0x3DA)
{

}

VideoGraphicsArray::~VideoGraphicsArray() {

}

void VideoGraphicsArray::WriteRegisters(uint8_t* registers) {

  // miscellaneous (MISC)
  miscPort.Write(*(registers++));

  // sequencer (SEQ)
  for (uint8_t i = 0; i < 5; i++) {
    // first write the index to the index port
    sequencerIndexPort.Write(i);

    // then write the data to the data port
    sequencerDataPort.Write(*(registers++));
  }

  // this is a security feature for doing something mutual exclusion
  // because the cathode ray tube controller can blow up if you will send the wrong data
  // so you need to unlock it send data and then lock it again
  //
  // for the 0x03 index, set the first bit to 1
  crtcIndexPort.Write(0x03);
  crtcDataPort.Write(crtcDataPort.Read() | 0x80);

  // for the 0x11 index, set the first bit to 0
  crtcIndexPort.Write(0x11);
  crtcDataPort.Write(crtcDataPort.Read() & ~0x80);

  // now if the data that we get which will be sent to the CRTC
  // we are over writing these valus again in this loop
  // so this would actually unlock the controller again
  //
  // It would lock the controller again if we got wrong data on in this registers here
  //
  // so we will make sure that we don't accidentally overwrite this again
  registers[0x03] = registers[0x03] | 0x80;
  registers[0x11] = registers[0x11] & ~0x80;


  // cathode ray tube controller (CRTC)
  for (uint8_t i = 0; i < 25; i++) {
    crtcIndexPort.Write(i);
    crtcDataPort.Write(*(registers++));
  }
  
  // graphics controller (GC)
  for (uint8_t i = 0; i < 9; i++) {
    graphicsControllerIndexPort.Write(i);
    graphicsControllerDataPort.Write(*(registers++));
  }

  // attribute controller (AC)
  for (uint8_t i = 0; i < 21; i++) {
    attributeControllerResetPort.Read();
    attributeControllerIndexPort.Write(i);
    attributeControllerWritePort.Write(*(registers++));
  }

  // when we are finished attribute controller,
  // reset the attribute controller again
  // and write 0x20 the attribute controller index
  attributeControllerResetPort.Read();
  attributeControllerIndexPort.Write(0x20);
}

bool VideoGraphicsArray::SupportsMode(uint32_t width, uint32_t height, uint32_t colordepth) {
  return width == 320 && height == 200 && colordepth == 8;
}

bool VideoGraphicsArray::SetMode(uint32_t width, uint32_t height, uint32_t colordepth) {
  // if the mode is not supported
  // we will try to set the mode to an unsupported mode
  // then we just return false
  if (!SupportsMode(width, height, colordepth)) {
    return false;
  }

  // find the graphics mode setting from the link:
  // https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
  unsigned char g_320x200x256[] =
  {
    /* MISC */
    0x63,
    /* SEQ */
    0x03, 0x01, 0x0F, 0x00, 0x0E,
    /* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28,	0x40, 0x96, 0xB9, 0xA3,
    0xFF,
    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF,
    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00,	0x00
  };

  // pass g_320x200x256 to the WriteRegisters
  WriteRegisters(g_320x200x256);

  return true;
}

uint8_t* VideoGraphicsArray::GetFrameBufferSegment() {
  // looking to the index 0x06 in the graphicsControllerIndexPort
  graphicsControllerIndexPort.Write(0x06);

  // get the graphicsControllerDataPort
  // we are only interested in the bits number three and four
  uint8_t segmentNumber = ((graphicsControllerDataPort.Read() >> 2) & 0x03);

  switch (segmentNumber) {
    // the compiler complains if you don't have a default value here
    default:

    // if this is 0 then we need to write 0x00000 to the memory location
    case 0: return (uint8_t*)0x00000;
    case 1: return (uint8_t*)0xA0000;
    case 2: return (uint8_t*)0x80000;
    case 3: return (uint8_t*)0xB8000;
  }
}

void VideoGraphicsArray::PutPixel(int32_t x, int32_t y, uint8_t colorIndex) {

  // make sure the coordinates are legal
  //
  // for example if you move the mouse to the right of the screen
  // then we try to draw the cross cursor and it's over beyond the screen
  // and that's not really what we want
  // because these pixels will just be drawn in the next line
  // so we will just put security here
  if (x < 0 || 320 <= x || y < 0 || 200 <= y) {
    return;
  }

  // asks the GetFrameBufferSegment where to put the pixel
  //
  // we haven't start the width and the height anywhere
  // so hard code this to 320 x 200
  uint8_t* pixelAddress = GetFrameBufferSegment() + 320 * y + x;

  // At this address we put the colorIndex
  *pixelAddress = colorIndex;
}

uint8_t VideoGraphicsArray::GetColorIndex(uint8_t r, uint8_t g, uint8_t b) {
  // do a little mock up here
  if (r == 0x00 && g == 0x00 && b == 0x00) return 0x00; // black
  if (r == 0x00 && g == 0x00 && b == 0xA8) return 0x01; // blue
  if (r == 0x00 && g == 0xA8 && b == 0x00) return 0x02; // green
  if (r == 0xA8 && g == 0x00 && b == 0x00) return 0x04; // red
  if (r == 0xFF && g == 0xFF && b == 0xFF) return 0x3F; // white
  return 0x00;
}

void VideoGraphicsArray::PutPixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b) {
  // call the other PutPixel(x, y, colorIntex)
  // with the same x, y but GetColorIndex for the RGB value
  PutPixel(x, y, GetColorIndex(r, g, b));
}

void VideoGraphicsArray::FillRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b) {
  for (int32_t Y = y; Y < y + h; Y++) {
    for (int32_t X = x; X < x + w; X++) {
      PutPixel(X, Y, r, g, b);
    }
  }
}
