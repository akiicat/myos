#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>

// #define GRAPHICSMODE

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
using namespace myos::net;

// write the string into the 0xb8000 memory address so that
// the graphic card will print the string on the screen 
// A character has 16 bits
//
//        4 bits           4 bits                   8 bits
// +-------------------------------------------------------------------+
// |  front ground  |   back ground  |             character           |
// +-------------------------------------------------------------------+
// |            high byte            |             low byte            |
// +-------------------------------------------------------------------+
//
// screen = 25 x 80 char
void printf(char* str) {
  static uint16_t* VideoMemory = (uint16_t*)0xb8000;

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
  foo[0] = hex[(key >> 4) & 0xF];
  foo[1] = hex[key & 0xF];
  printf(foo);
}

void printfHex16(uint16_t key) {
  printfHex((key >> 8) & 0xFF);
  printfHex( key & 0xFF);
}

void printfHex32(uint32_t key) {
  printfHex((key >> 24) & 0xFF);
  printfHex((key >> 16) & 0xFF);
  printfHex((key >> 8) & 0xFF);
  printfHex( key & 0xFF);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler {
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
      uint16_t* VideoMemory = (uint16_t*)0xb8000;
      x = 40;
      y = 12;
      VideoMemory[80 * y + x] = ((VideoMemory[80 * y + x] & 0x0F00) << 4)
                              | ((VideoMemory[80 * y + x] & 0xF000) >> 4)
                              |  (VideoMemory[80 * y + x] & 0x00FF);        
    }

    virtual void OnMouseMove(int xoffset, int yoffset) {
      // display mouse on the screen
      static uint16_t* VideoMemory = (uint16_t*)0xb8000;
      // flip back the old position to the original state
      VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4
                              | (VideoMemory[80 * y + x] & 0xF000) >> 4
                              | (VideoMemory[80 * y + x] & 0x00FF);

      // move the cursor
      x += xoffset;
      if (x >= 80) x = 79;
      if (x < 0) x = 0;
      y += yoffset;
      if (y >= 25) y = 24;
      if (y < 0) y = 0;

      // flip the new position
      VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4
                              | (VideoMemory[80 * y + x] & 0xF000) >> 4
                              | (VideoMemory[80 * y + x] & 0x00FF);
    }
};

void sysprintf(char* str) {
  asm("int $0x80" : : "a" (4), "b" (str));
}

void taskA() { while(true) { sysprintf("A"); } }
void taskB() { while(true) { sysprintf("B"); } }

// Write a custom contructor
typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
  for (constructor* i = &start_ctors; i != &end_ctors; i++) {
    (*i)();
  }
}

extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/) {
  printf("Hello World!\n");

  GlobalDescriptorTable gdt;

  // instantiate one
  // the question really is what values do we pass here
  // so I will just hard code this to start at 10MB
  // but what should we use as the size
  // we have given the virtaul machine 64 MB so we could use just 54 MB as size here
  // but that wouldn't be protable so what I'm going to do instead
  // as far as I know there is a BIOS interrupt which will give you the size of the memory but I really
  // don't want to got into interrupts inside the C++ code
  // so there's actually a better way to get this
  //
  // so here's a situation is this
  // we are getting this pointer to the multi boot structure from grub right
  // and so far we haven't done anything with it
  // but when you look into the multibook.h from the group project also known as linux
  // (https://www.gnu.org/software/grub/manual/multiboot/html_node/multiboot_002eh.html)
  // then you will find this structure here the multiboot_info
  // and there you see how this data is structured
  // so you could just copy that multiboot.h and include it
  // and then you could use multiboot_info pointer instead of void pointer
  //
  // is I'll take this pointer to the multiboot_structure at 8 bytes and
  // cast the result to in you 32 pointer
  uint32_t *memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);

  // hard coding heap to 10MB
  size_t heap = 10*1024*1024;

  // and here as size I will just take the content of that pointer
  // multiply it with 1024 because it is in kilobytes
  // and I will substruct the address where the heap starts
  // and I will also substract 10 kilobytes of padding behind the heap
  MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
  printf("heap: 0x");
  printfHex((heap >> 24) & 0xFF);
  printfHex((heap >> 16) & 0xFF);
  printfHex((heap >>  8) & 0xFF);
  printfHex((heap      ) & 0xFF);

  void* allocated = memoryManager.malloc(1024);
  printf("\nallocated: 0x");
  printfHex(((size_t)allocated >> 24) & 0xFF);
  printfHex(((size_t)allocated >> 16) & 0xFF);
  printfHex(((size_t)allocated >>  8) & 0xFF);
  printfHex(((size_t)allocated      ) & 0xFF);
  printf("\n");

  // the reason why I instantiated it up there is because
  // the interrupt handler will need to talk to the taskManager to do the scheduling
  TaskManager taskManager;
#ifdef AB_TASK
  Task task1(&gdt, taskA);
  Task task2(&gdt, taskB);
  taskManager.AddTask(&task1);
  taskManager.AddTask(&task2);
#endif

  InterruptManager interrupts(0x20, &gdt, &taskManager);
  SyscallHandler syscalls(&interrupts, 0x80);

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

#ifdef GRAPHICSMODE
  VideoGraphicsArray vga;
#endif

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

#ifdef ATA
  // interrupt 14
  printf("\nS-ATA primary master: ");
  AdvancedTechnologyAttachment ata0m(true, 0x1F0); // master, portBase: 0x1F0
  ata0m.Identify();

  printf("\nS-ATA primary slave: ");
  AdvancedTechnologyAttachment ata0s(false, 0x1F0); // slave
  ata0s.Identify();

  // write something to the disk and flush. after that read it.
  ata0s.Write28(0, (uint8_t*)"Hello World", 13);
  ata0s.Flush();
  ata0s.Read28(0, 13);

  // interrupt 15
  printf("\nS-ATA secondary master: ");
  AdvancedTechnologyAttachment ata1m(true, 0x170); // master, portBase: 0x1F0
  ata1m.Identify();

  printf("\nS-ATA secondary slave: ");
  AdvancedTechnologyAttachment ata1s(false, 0x170); // slave
  ata1s.Identify();

  // third portBase: 0x1E8
  // fourth portBase: 0x168
#endif

  amd_am79c973* eth0 = (amd_am79c973*)(drvManager.drivers[2]);

  // This was an IP address that virualbox will accept 10.0.2.15
  // IP 10.0.2.15
  uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15; // ip
  uint32_t ip_be = ((uint32_t)ip4 << 24)
                 | ((uint32_t)ip3 << 16)
                 | ((uint32_t)ip2 << 8)
                 | ((uint32_t)ip1);

  // Told the network card that this is our IP, so the other layers
  // can request it.
  eth0->SetIPAddress(ip_be);

  // Here, we attach Etherframe to the driver.
  EtherFrameProvider etherframe(eth0);

  // The AddressResolutionProtocol is attached to the etherframe.
  AddressResolutionProtocol arp(&etherframe);
  // etherframe.Send(0xFFFFFFFFFFFF, 0x0608, (uint8_t*)"FOO", 3); // 0x0806 ARP

  // IP 10.0.2.2 of the default gateway
  uint8_t gip1 = 10, gip2 = 0, gip3 = 2, gip4 = 2;
  uint32_t gip_be = ((uint32_t)gip4 << 24)
                  | ((uint32_t)gip3 << 16)
                  | ((uint32_t)gip2 << 8)
                  | ((uint32_t)gip1);

  uint8_t subnet1 = 255, subnet2 = 255, subnet3 = 255, subnet4 = 0;
  uint32_t subnet_be = ((uint32_t)subnet4 << 24)
                     | ((uint32_t)subnet3 << 16)
                     | ((uint32_t)subnet2 << 8)
                     | ((uint32_t)subnet1);

  InternetProtocolProvider ipv4(&etherframe, &arp, gip_be, subnet_be);


  // activate the interrupts which really be the last thing we do
  //
  // because after that you just don't know when the processor jumps out of the kernel stack and into the tasks
  // it never goes back to the kernel stack so everything after this interrupts activate then might not be executed anymore
  // so that's why this really should be the last thing here
  interrupts.Activate();

  // ARP: We can get an answer only after the interrputs are activated.
  // Because when we get an answer, this is done through an interrupt handler.
  // printf("\n");
  printf("\n\n\n\n\n\n\n\n");
  // arp.Resolve(gip_be); // request gateway ip

  // 0x0008 is big endian encoding for ipv4
  ipv4.Send(gip_be, 0x0008, (uint8_t*) "foobar", 6); // send something to the gateway

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
