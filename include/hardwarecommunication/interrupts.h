#ifndef __MYOS__HARDWARECOMMUNICATION__INTERRUPTS_H
#define __MYOS__HARDWARECOMMUNICATION__INTERRUPTS_H

#include <gdt.h>
#include <multitasking.h>
#include <common/types.h>
#include <hardwarecommunication/port.h>

namespace myos {

  namespace hardwarecommunication {

    class InterruptManager;

    class InterruptHandler {
      protected:
        myos::common::uint8_t interruptNumber;
        InterruptManager* interruptManager;

      public:
        InterruptHandler(myos::common::uint8_t interruptNumber, InterruptManager* interruptManager);
        ~InterruptHandler();

        // must virtual mode:
        //   when inherit this class, the parent's function will be overwrited by the child's one.
        // if not add `virtual` on declaration:
        //   The parent's function will not be overwirted by the child's function.
        virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);

    };

    class InterruptManager {
      friend class InterruptHandler;

      protected:

        // put a current interrupt manager here to ensure we only have one active interrupte mananger
        static InterruptManager* ActiveInterruptManager;

        InterruptHandler* handlers[256];

        TaskManager *taskManager;

        /*
         * Interrupt Descriptor Table Entry is called Gate Descriptor
         *
         * - handlerAddressLowBits:
         *   Lower part of the interrupt function's offset address (also known as pointer).
         *
         * - gdt_codeSegmentSelector:
         *   Selector of the interrupt function (to make sense - the kernel's selector).
         *   The selector is a 16 bit value and must point to a valid descriptor in your GDT.
         *   The selector's descriptor's DPL field has to be 0 so the iret instruction won't
         *   throw a #GP exeption when executed.
         *
         * - access:
         *
         *     7                           0
         *   +---+---+---+---+---+---+---+---+
         *   | P |  DPL  | S |    GateType   |
         *   +---+---+---+---+---+---+---+---+
         *
         *   - P(Present): Set to 0 for unused interrupts.
         *   - DPL(Descriptor Privilege Level): Gate call protection.
         *     Specifies which privilege Level the calling Descriptor
         *     minimum should have. So hardware and CPU interrupts can
         *     be protected from being called out of userspace. (Dual mode)
         *   - S(Storage Segment): Set to 0 for interrupt and trap gates.
         *   - GateType: 
         *     Possible IDT gate types :
         *       0b0101 0x5 5   80386 32 bit task gate
         *       0b0110 0x6 6   80286 16-bit interrupt gate
         *       0b0111 0x7 7   80286 16-bit trap gate
         *       0b1110 0xE 14	80386 32-bit interrupt gate
         *       0b1111 0xF 15	80386 32-bit trap gate
         *    Interrupt Types:
         *      +-----------+-----------------------+
         *      | Interrupt |         Trap          |
         *      +-----------+-----------+-----------+
         *      | External  | Internal  | Software  |
         *      | Interrupt | Interrupt | Interrupt |
         *      +-----------+-----------+-----------+
         *    - Interrupt: Hardware Interrupt, External Interrup
         *        IVT Offset | INT # | IRQ # | Description
         *        -----------+-------+-------+------------------------------
         *        0x0020     | 0x08  | 0     | PIT
         *        0x0024     | 0x09  | 1     | Keyboard
         *        0x0028     | 0x0A  | 2     | 8259A slave controller
         *        0x002C     | 0x0B  | 3     | COM2 / COM4
         *        0x0030     | 0x0C  | 4     | COM1 / COM3
         *        0x0034     | 0x0D  | 5     | LPT2
         *        0x0038     | 0x0E  | 6     | Floppy controller
         *        0x003C     | 0x0F  | 7     | LPT1
         *    - Trap: CPU Interrupt, Internal Interrupt
         *        IVT Offset | INT #     | Description
         *        -----------+-----------+-----------------------------------
         *        0x0000     | 0x00      | Divide by 0
         *        0x0004     | 0x01      | Reserved
         *        0x0008     | 0x02      | NMI Interrupt
         *        0x000C     | 0x03      | Breakpoint (INT3)
         *        0x0010     | 0x04      | Overflow (INTO)
         *        0x0014     | 0x05      | Bounds range exceeded (BOUND)
         *        0x0018     | 0x06      | Invalid opcode (UD2)
         *        0x001C     | 0x07      | Device not available (WAIT/FWAIT)
         *        0x0020     | 0x08      | Double fault
         *        0x0024     | 0x09      | Coprocessor segment overrun
         *        0x0028     | 0x0A      | Invalid TSS
         *        0x002C     | 0x0B      | Segment not present
         *        0x0030     | 0x0C      | Stack-segment fault
         *        0x0034     | 0x0D      | General protection fault
         *        0x0038     | 0x0E      | Page fault
         *        0x003C     | 0x0F      | Reserved
         *        0x0040     | 0x10      | x87 FPU error
         *        0x0044     | 0x11      | Alignment check
         *        0x0048     | 0x12      | Machine check
         *        0x004C     | 0x13      | SIMD Floating-Point Exception
         *        0x00xx     | 0x14-0x1F | Reserved
         *        0x0xxx     | 0x20-0xFF | User definable                     (Software Interrupt)
         *
         *   
         * - handlerAddressHighBits:
         *   Higher part of the offset.
         *
         * The handlerAddressLowBits and handlerAddressHighBits are a 32 bit value, split in two parts.
         * It represents the address of the entry point of the ISR.
         */
        struct GateDescriptor {
          myos::common::uint16_t handlerAddressLowBits;    // offset bits 0..15
          myos::common::uint16_t gdt_codeSegmentSelector;  // a code segment selector in GDT or LDT
          myos::common::uint8_t reserved;                  // unused, set to 0
          myos::common::uint8_t access;                    // type and attributes
          myos::common::uint16_t handlerAddressHighBits;   // offset bits 16..31
        } __attribute__((packed));

        /*
         * Interrupt Descriptor Table (IDT)
         *
         * The table in Real Mode is called Interrupt Vector Table (IVT)
         * The table in Protect Mode is called Interrupt Descriptor Table (IDT)
         *
         * It can contain more or less than 256 entries. More entries are ignored.
         * When an interrupt or exception is invoked whose entry is not present,
         * a GPF is raised that tells the number of the missing IDT entry, and
         * even whether it was hardware or software interrupt. There should
         * therefore be at least enough entries so a GPF can be caught.
         */
        static GateDescriptor interruptDescriptorTable[256];

        /*
         * to tell the processor to use this
         *
         *   Location of IDT (address and size) is kept in the IDTR register of
         * the CPU, which can be loaded/stored using LIDT, SIDT instructions.
         * 
         * Limit  0..15 Defines the length of the IDT in bytes - 1 (minimum value is 100h, a value of 1000h means 200h interrupts).
         * Base  16..47	This 32 bits are the linear address where the IDT starts (INT 0)
         */
        struct InterruptDescriptorTablePointer {
          myos::common::uint16_t size;
          myos::common::uint32_t base;
        } __attribute__((packed));

        // set the entries in the interrupt descriptor table
        static void SetInterruptDescriptorTableEntry(
            myos::common::uint8_t interruptNumber,
            myos::common::uint16_t codeSegmentSelectorOffset,
            void (*handler)(),
            myos::common::uint8_t DescriptorPrivilegeLevel,
            myos::common::uint8_t DescriptorType
        );

        Port8BitSlow picMasterCommand;
        Port8BitSlow picMasterData;
        Port8BitSlow picSlaveCommand;
        Port8BitSlow picSlaveData;

      public:
        // input GDT table
        InterruptManager(myos::GlobalDescriptorTable* gdt, myos::TaskManager* taskManager);
        ~InterruptManager();

        void Activate();
        void Deactivate();

        static myos::common::uint32_t HandleInterrupt(myos::common::uint8_t interruptNumber, myos::common::uint32_t esp);

        // call the non-static function to handle interrupt ActiveInterruptManager
        myos::common::uint32_t DoHandleInterrupt(myos::common::uint8_t interruptNumber, myos::common::uint32_t esp);

        static void IgnoreInterruptRequest();
        static void HandleInterruptRequest0x00(); // timer interrupt
        static void HandleInterruptRequest0x01(); // keyboard interrupt
        static void HandleInterruptRequest0x0C(); // mouse interrupt
    };

  }

}

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
