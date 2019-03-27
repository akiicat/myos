/*
 * Intel defined 3 types of tables:
 *   - Interrupt Descriptor Table (IDT which is supplants the IVT)
 *   - Global Descriptor Table (GDT)
 *   - Local Descriptor Table (LDT)
 *
 * Segment: a logically contiguous chunk of memory with consistent
 *   properties (CPU's speaking)
 * Segment Register: a register of your CPU that refers to a segment
 *   for a special use (e.g. SS, CS, DS ...)
 * Selector: a reference to a descriptor you can load into a segment
 *   register; the selector is an offset of a descriptor table entry.
 *   These entries are 8 bytes long. Therefore, bits 3 and up only
 *   declare the descriptor table entry offset, while bit 2 specifies
 *   if this selector is a GDT or LDT selector (LDT - bit set, GDT -
 *   bit cleared), and bits 0 - 1 declare the ring level that needs
 *   to corespond to the descriptor table entry's DPL field. If it
 *   doesn't, a General Protection Fault occurs; if it does corespond
 *   then the CPL level of the selector used changed accordingly.
 * Descriptor: a memory structure (part of a table) that tells the
 *  CPU the attributes of a given segment
 */

#ifndef __GDT_H
#define __GDT_H

#include "types.h"

class GlobalDescriptorTable {
  public:
    class SegmentDescriptor {
      private:
        uint16_t limit_lo;
        uint16_t base_lo;
        uint8_t base_hi;
        uint8_t type;
        uint8_t flags_limit_hi;
        uint8_t base_vhi;
      public:
        SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type);
        uint32_t Base();
        uint32_t Limit();

    } __attribute__((packed));

    SegmentDescriptor nullSegmentSelector;
    SegmentDescriptor unusedSegmentSelector;
    SegmentDescriptor codeSegmentSelector;
    SegmentDescriptor dataSegmentSelector;

  public:
    GlobalDescriptorTable();
    ~GlobalDescriptorTable();

    uint16_t CodeSegmentSelector();
    uint16_t DataSegmentSelector();
};

#endif
