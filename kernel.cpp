
#include "types.h"
#include "gdt.h"
#include "interrupts.h"
#include "keyboard.h"

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
  printf("Hello World!");

  GlobalDescriptorTable gdt;
  InterruptManager interrupts(&gdt);

  KeyboardDriver keyboard(&interrupts);

  interrupts.Activate();

  while(1);
}
