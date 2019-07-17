/*
 * Global Descriptor Table is a Segment Table
 *
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

#ifndef __MYOS__GDT_H
#define __MYOS__GDT_H

#include <common/types.h>

namespace myos {

  class GlobalDescriptorTable {
    public:
      class SegmentDescriptor {
        private:
          common::uint16_t limit_lo;
          common::uint16_t base_lo;
          common::uint8_t base_hi;
          common::uint8_t type;
          common::uint8_t flags_limit_hi;
          common::uint8_t base_vhi;

        public:
          // Initialize Segment Table
          SegmentDescriptor(common::uint32_t base, common::uint32_t limit, common::uint8_t type);

          // Get Segment Table detail
          common::uint32_t Base();
          common::uint32_t Limit();

        // The packed type attribute specifies that a type must have the smallest possible alignment.
        // 叫compiler不要為我們做對齊的最佳化
      } __attribute__((packed));

    private:

      // for the segment table security
      SegmentDescriptor nullSegmentSelector;
      SegmentDescriptor unusedSegmentSelector;

      // the code segment and data segment should be placed in LDT
      SegmentDescriptor codeSegmentSelector;
      SegmentDescriptor dataSegmentSelector;

    public:
      GlobalDescriptorTable();
      ~GlobalDescriptorTable();

      // This code is supposed to be give us the `offset` of the code segment descriptor and one for data segment descriptor
      common::uint16_t CodeSegmentSelector();
      common::uint16_t DataSegmentSelector();
  };

}

#endif
