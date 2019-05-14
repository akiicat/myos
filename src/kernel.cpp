#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>

// #define GRAPHICSMODE

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;


// 將 string 寫到特定的記憶體位置 0xb8000
// 顯示卡則會去此位置抓取值，將文字 render 到螢幕上
//
// 以 16 個 bit 為單位組成一個文字：
//   - high byte:
//     - 4bit: front ground
//     - 4bit: back ground
//   - low byte:
//     - 8bit: char
//
// screen = 25 x 80 char
void printf(char *str) {
  static uint16_t * VideoMemory = (uint16_t *) 0xb8000;

  static uint8_t x = 0, y = 0;

  for (int i = 0; str[i] != '\0'; ++i) {

    switch (str[i]) {
      case '\n':
        y++;
        x = 0;
        break;
      default:
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
        x++;
    }


    if (x >= 80) {
      y++;
      x = 0;
    }

    if (y >= 25) {
      for (y = 0; y < 25; y++) {
        for (x = 0; x < 80; x++) {
          VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
          x++;
        }
      }

      x = 0;
      y = 0;
    }

  }
}

void printfHex(uint8_t key) {
  char* foo = "00";
  char* hex = "0123456789ABCDEF";
  foo[0] = hex[(key >> 4) & 0x0F];
  foo[1] = hex[key & 0x0F];
  printf(foo);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler {
    int8_t x, y;
  public:
    void OnKeyDown(char c) {
      char* foo = " ";
      foo[0] = c;
      printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler {
    int8_t x, y;
  public:
    MouseToConsole() {
      // initialize the cursor
      static uint16_t * VideoMemory = (uint16_t *) 0xb8000;
      x = 40;
      y = 12;
      VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4)
                              | ((VideoMemory[80 * y + x] & 0x0F00) << 4)
                              |  (VideoMemory[80 * y + x] & 0x00FF);
    }

    void OnMouseMove(int xoffset, int yoffset) {
      // display mouse on the screen
      static uint16_t * VideoMemory = (uint16_t *) 0xb8000;

      // flip back the old position to the original state
      VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4)
                              | ((VideoMemory[80 * y + x] & 0x0F00) << 4)
                              | ((VideoMemory[80 * y + x] & 0x00FF));

      // move the cursor
      x += xoffset;
      if (x < 0) x = 0;
      if (x >= 80) x = 79;

      y += yoffset;
      if (y < 0) y = 0;
      if (y >= 25) y = 24;

      // flip the new position
      VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0xF000) >> 4)
                              | ((VideoMemory[80 * y + x] & 0x0F00) << 4)
                              | ((VideoMemory[80 * y + x] & 0x00FF));
    }
};

// Write a custom contructor
typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
  for (constructor* i = &start_ctors; i != &end_ctors; i++) {
    (*i)();
  }
}


extern "C" void kernelMain(void *multiboot_structure, uint16_t magicnumber) {
  printf("Hello World!\n");

  GlobalDescriptorTable gdt;
  InterruptManager interrupts(&gdt);

  printf("Initializing Hardware, Stage 1\n");


#ifdef GRAPHICSMODE
  Desktop desktop(320, 200, 0x00, 0x00, 0xA8);
#endif

  DriverManager drvManager;

#ifdef GRAPHICSMODE
  KeyboardDriver keyboard(&interrupts, &desktop);
#else
  PrintfKeyboardEventHandler kbhandler;
  KeyboardDriver keyboard(&interrupts, &kbhandler);
#endif
  drvManager.AddDriver(&keyboard);

#ifdef GRAPHICSMODE
  MouseDriver mouse(&interrupts, &desktop);
#else
  MouseToConsole mousehandler;
  MouseDriver mouse(&interrupts, &mousehandler);
#endif
  drvManager.AddDriver(&mouse);

  PeripheralComponentInterconnectController PCIController;
  PCIController.SelectDrivers(&drvManager, &interrupts);

  VideoGraphicsArray vga;

  printf("Initializing Hardware, Stage 2\n");
  drvManager.ActivateAll();

  printf("Initializing Hardware, Stage 3\n");

#ifdef GRAPHICSMODE
  vga.SetMode(320, 200, 8);

  Window win1(&desktop, 10, 10, 20, 20, 0xA8, 0x00, 0x00);
  desktop.AddChild(&win1);
  Window win2(&desktop, 40, 15, 30, 30, 0x00, 0xA8, 0x00);
  desktop.AddChild(&win2);
#endif

  // activate the interrupts which really be the last thing we do
  //
  // because after that you just don't know when the processor jumps out of the kernel stack and into the tasks
  // it never goes back to the kernel stack so everything after this interrupts activate then might not be executed anymore
  // so that's why this really should be the last thing here
  interrupts.Activate();

  // when we start the multitasking,
  // this loop will never be executed anymore
  while(1) {
#ifdef GRAPHICSMODE
    // draw the desktop in the loop
    //
    // this is after we activate it to interrupts
    // so it's really not a good idea to redraw the desktop inside this loop
    //
    // that is why this desktop draw here will break when we go into multitasking
    // and then you should just have a different task for the redrawing here
    desktop.Draw(&vga);
#endif
  }
}
