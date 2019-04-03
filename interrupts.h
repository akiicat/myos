#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#include "types.h"
#include "port.h"
#include "gdt.h"

class InterruptManager {
  protected:
    struct GateDescriptor {
      uint16_t handlerAddressLowBits;
      uint16_t gdt_codeSegmentSelector;
      uint8_t reserved;
      uint8_t access;
      uint16_t handlerAddressHighBits;

    } __attribute__((packed));

    static GateDescriptor interruptDescriptorTable[256];

    struct InterruptDescriptorTablePointer {
      uint16_t size;
      uint32_t base;
    } __attribute__((packed));

    static void SetInterruptDescriptorTableEntry(
        uint8_t interruptNumber,
        uint16_t codeSegmentSelectorOffset,
        void (*handler)(),
        uint8_t DescriptorPrivilegeLevel,
        uint8_t DescriptorType
    );

    Port8BitSlow picMasterCommand;
    Port8BitSlow picMasterData;
    Port8BitSlow picSlaveCommand;
    Port8BitSlow picSlaveData;


  public:
    InterruptManager(GlobalDescriptorTable* gdt);
    ~InterruptManager();

    void Activate();

    static uint32_t handleInterrupt(uint8_t interruptNumber, uint32_t esp);

    static void IgnoreInterruptRequest();
    static void HandleInterruptRequest0x00(); // timer
    static void HandleInterruptRequest0x01(); // keyboard
};

#endif


/*
 * $ nm interrupts.o
 *          U _GLOBAL_OFFSET_TABLE_
 * 00000000 T __x86.get_pc_thunk.ax
 * 00000000 T __x86.get_pc_thunk.bx
 *          U _Z6printfPc
 *          U _ZN12Port8BitSlow5WriteEh
 *          U _ZN12Port8BitSlowC1Et
 *          U _ZN12Port8BitSlowD1Ev
 * 000002dc T _ZN16InterruptManager15handleInterruptEhj  # the name of the handle interrupt static method
 *          U _ZN16InterruptManager22IgnoreInterruptRequestEv
 * 00000000 B _ZN16InterruptManager24interruptDescriptorTableE
 *          U _ZN16InterruptManager26HandleInterruptRequest0x00Ev
 *          U _ZN16InterruptManager26HandleInterruptRequest0x01Ev
 * 00000000 T _ZN16InterruptManager32SetInterruptDescriptorTableEntryEhtPFvvEhh
 * 000002ca T _ZN16InterruptManager8ActivateEv
 * 0000009a T _ZN16InterruptManagerC1EP21GlobalDescriptorTable
 * 0000009a T _ZN16InterruptManagerC2EP21GlobalDescriptorTable
 * 0000026c T _ZN16InterruptManagerD1Ev
 * 0000026c T _ZN16InterruptManagerD2Ev
 *          U _ZN21GlobalDescriptorTable19CodeSegmentSelectorEv
 *
 */
