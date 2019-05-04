/*
 * https://wiki.osdev.org/Global_Descriptor_Table
 *
 * 63     56 55 52 51      48 47      40 39     32 | 31    16 15      0
 * Base24:31 Flags Limit16:19 AccessByte Base16:23 | Base0:15|Limit0:15|
 *   1B     | 0.5B    0.5B   |   1B     |  1B      |   2B    |   2B    | (B: byte)
 *     7    |       6        |     5    |     4    |  3 | 2  |  1 | 0  | (target)
 *
 * flags: target[6] & 0xF0
 *    7  6  5  4
 *   Gr Sz  0  0
 *
 *   Gr: Granularity bit. If 0 the limit is in 1 B blocks (byte granularity),
 *       if 1 the limit is in 4 KiB blocks (page granularity).
 *   Sz: Size bit. If 0 the selector defines 16 bit protected mode. If 1 it
 *       defines 32 bit protected mode. You can have both 16 bit and 32 bit
 *       selectors at once.
 *
 *
 * AccessByte: target[5]
 *   7  6  5  4  3  2  1  0
 *  Pr Privl  S Ex DC RW Ac
 *
 *  Pr: Present bit. This must be 1 for all valid selectors.
 *  Privl: Privilege, 2 bits. Contains the ring level, 0 = highest (kernel),
 *         3 = lowest (user applications). (Dual mode)
 *  S: Descriptor type. This bit should be set for code or data segments and
 *     should be cleared for system segments (eg. a Task State Segment)
 *  Ex: Executable bit. If 1 code in this segment can be executed, ie. a code
 *      selector. If 0 it is a data selector.
 *  DC: Direction bit / Conforming bit.
 *    - Direction bit for data selectors: Tells the direction. 0 the segment
 *      grows up. 1 the segment grows down, ie. the offset has to be greater than the limit.
 *    - Conforming bit for code selectors:
 *      - If 1 code in this segment can be executed from an equal or lower privilege level.
 *        For example, code in ring 3 can far-jump to conforming code in a ring 2 segment.
 *        The privl-bits represent the highest privilege level that is allowed to execute
 *        the segment. For example, code in ring 0 cannot far-jump to a conforming code
 *        segment with privl==0x2, while code in ring 2 and 3 can. Note that the privilege
 *        level remains the same, ie. a far-jump form ring 3 to a privl==2-segment remains
 *        in ring 3 after the jump.
 *      - If 0 code in this segment can only be executed from the ring set in privl.
 *  RW: Readable bit / Writable bit.
 *    - Readable bit for code selectors: Whether read access for this segment is allowed.
 *      Write access is never allowed for code segments.
 *    - Writable bit for data selectors: Whether write access for this segment is allowed.
 *      Read access is always allowed for data segments.
 *  Ac: Accessed bit. Just set to 0. The CPU sets this to 1 when the segment is accessed.
 *
 */

#include <gdt.h>

using namespace myos;
using namespace myos::common;

/* What should I put in my GDT?
 *
 *   The null descriptor which is never referenced by the processor. Certain emulators, like
 * Bochs, will complain about limit exceptions if you do not have one present. Some use
 * this descriptor to store a pointer to the GDT itself (to use with the LGDT instruction).
 * The null descriptor is 8 bytes wide and the pointer is 6 bytes wide so it might just be
 * the perfect place for this.
 *
 *   A code segment descriptor (for your kernel, it should have type 0x9A)
 *
 *  A data segment descriptor (you can't write to a code segment, so add this with type 0x92)
 */
GlobalDescriptorTable::GlobalDescriptorTable() :
  nullSegmentSelector(0, 0, 0),
  unusedSegmentSelector(0, 0, 0),
  codeSegmentSelector(0, 64*1024*1024, 0x9A), // 64MiB for code segment
  dataSegmentSelector(0, 64*1024*1024, 0x92)  // 64MiB for data segment
{
  uint32_t i[2];

  i[1] = (uint32_t)this;
  i[0] = sizeof(GlobalDescriptorTable) << 16; // 65535 entries = 2 ^ 19 bytes

  // Telling the CPU where the table stands
  //   lgdt: load global descriptor table
  //   %0: output operand
  //   %1: input operand
  //   p: a valid memory address (pointer)
  asm volatile("lgdt (%0)": :"p" (((uint8_t *) i) + 2)); 

}

GlobalDescriptorTable::~GlobalDescriptorTable() {

}

uint16_t GlobalDescriptorTable::DataSegmentSelector() {
  return (uint8_t *) &dataSegmentSelector - (uint8_t *) this;
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
  return (uint8_t *) &codeSegmentSelector - (uint8_t *) this;
}

GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t flags) {
  uint8_t *target = (uint8_t *)this;

  // check it!!!
  // limit    target[6] target[1:0]
  // 0x00000000   0x40      0x0000
  // 0x00000FFF   0x40      0x0FFF
  // 0x00001000   0xC0      0x0000
  // 0x00001FFE   0xC0      0x0000
  // 0x00001FFF   0xC0      0x0001
  // 0x54321000   0xC5      0x4320
  // 0x54321FFF   0xC5      0x4321
  // Adjust granularity if required
  if (limit <= 65536) {
    target[6] = 0x40;
  }
  else {
    if ((limit & 0xFFF) != 0xFFF) 
      limit = (limit >> 12) - 1; // It is not allowed that limit is not end with 0xFFF, so manually fix it, but why subtrace 1
    else
      limit = limit >> 12;

    target[6] = 0xC0;
  }

  // Encode the limit
  target[0] = limit & 0xFF;
  target[1] = limit >> 8 & 0xFF;
  target[6] |= limit >> 16 & 0xF;;

  // Encode the base
  target[2] = base & 0xFF;
  target[3] = (base >> 8) & 0xFF;
  target[4] = (base >> 16) & 0xFF;
  target[7] = (base >> 24) & 0xFF;

  target[5] = flags;
}

// Decode the base
uint32_t GlobalDescriptorTable::SegmentDescriptor::Base() {
  uint8_t *target = (uint8_t *)this;
  uint32_t result = target[7];
  result = (result << 8) + target[4];
  result = (result << 8) + target[3];
  result = (result << 8) + target[2];
  return result;
}

// Decode the limit
uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit() {
  uint8_t *target = (uint8_t *)this;
  uint32_t result = target[6] & 0xF;
  result = (result << 8) + target[1];
  result = (result << 8) + target[0];

  if ((target[6] & 0xC0) == 0xC0)
    result = (result << 12) | 0xFFF;

  return result;
}

