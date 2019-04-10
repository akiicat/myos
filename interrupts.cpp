#include "interrupts.h"

// forward definition of printf
void printf(char* str);

InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];

InterruptManager* InterruptManager::ActiveInterruptManager = 0;

/* set entry to the interrupt ignore interrupt request
 *
 * DescriptorPrivilegeLevel: Ring 0, 1, 2, 3
 * DescriptorType: 0xE
 *     Possible IDT gate types :
 *       0b0101 0x5 5   80386 32 bit task gate
 *       0b0110 0x6 6   80286 16-bit interrupt gate
 *       0b0111 0x7 7   80286 16-bit trap gate
 *       0b1110 0xE 14	80386 32-bit interrupt gate
 *       0b1111 0xF 15	80386 32-bit trap gate
 *
 */
void InterruptManager::SetInterruptDescriptorTableEntry(
    uint8_t interruptNumber,
    uint16_t codeSegmentSelectorOffset,
    void (*handler)(),
    uint8_t DescriptorPrivilegeLevel,
    uint8_t DescriptorType)
{
  const uint8_t IDT_DESC_PRESENT = 0x80;
  interruptDescriptorTable[interruptNumber].handlerAddressLowBits = ((uint32_t)handler) & 0xFFFF;
  interruptDescriptorTable[interruptNumber].handlerAddressHighBits = (((uint32_t)handler) >> 16) & 0xFFFF;
  interruptDescriptorTable[interruptNumber].gdt_codeSegmentSelector = codeSegmentSelectorOffset;
  // access type is 0x8E for ignore interrupt request
  interruptDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | DescriptorType | ((DescriptorPrivilegeLevel & 3) << 5);
  interruptDescriptorTable[interruptNumber].reserved = 0;
}

InterruptManager::InterruptManager(GlobalDescriptorTable* gdt)
: picMasterCommand(0x20),
  picMasterData(0x21),
  picSlaveCommand(0xA0),
  picSlaveData(0xA1)
{
  // Get GDT code segment
  uint16_t CodeSegment = gdt->CodeSegmentSelector();
  const uint8_t IDT_INTERRUPT_GATE = 0xE;

  /*
   * set all entries to ignore interrrupt request.
   *
   * If we get an interrupt and we don't have a hanlder for it,
   * we get a global portection fault (GPF) and the operating system crash.
   *
   * Params:
   * - DescriptorPrivilegeLevel: 0 (Ring 0)
   * - IDT_INTERRUPT_GATE: 0xE (32-bit interrupt gate)
   */
  for (uint16_t i = 0; i < 256; i++) {
    SetInterruptDescriptorTableEntry(i, CodeSegment, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
  }

  // The first Programmable Interrupt Controller (PIC)
  SetInterruptDescriptorTableEntry(0x20, CodeSegment, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE); // timer
  SetInterruptDescriptorTableEntry(0x21, CodeSegment, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE); // keyboard

  picMasterCommand.Write(0x11);
  picSlaveCommand.Write(0x11);

  picMasterData.Write(0x20);
  picSlaveData.Write(0x28);

  picMasterData.Write(0x04);
  picSlaveData.Write(0x02);

  picMasterData.Write(0x01);
  picSlaveData.Write(0x01);

  picMasterData.Write(0x00);
  picSlaveData.Write(0x00);

  // after creating the table, tell the processor to use it
  InterruptDescriptorTablePointer idt;
  idt.size = 256 * sizeof(GateDescriptor) - 1;    // IDT size
  idt.base = (uint32_t)interruptDescriptorTable;  // pointer to IDT

  // tell the processor to load IDT
  asm volatile("lidt %0" : : "m" (idt)); // lidt: load IDT
}

InterruptManager::~InterruptManager() {

}

void InterruptManager::Activate() {

  if (ActiveInterruptManager != 0) {
    ActiveInterruptManager->Deactivate();
  }

  ActiveInterruptManager = this;

  asm("sti"); // sti: start interrupts
}

void InterruptManager::Deactivate() {

  if (ActiveInterruptManager == this) {
    ActiveInterruptManager = 0;
    asm("cli"); // sti: start interrupts
  }
}

uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp) {

  if (ActiveInterruptManager != 0) {
    return ActiveInterruptManager->DoHandleInterrupt(interruptNumber, esp);
  }

  printf(" INTERUPT");

  // return the same stack pointer for disable task switching
  // since we don't have multiple processes
  return esp;
}

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp) {

  if (interruptNumber != 0x20) {
    printf(" INTERUPT");
  }

  if (0x20 <= interruptNumber && interruptNumber < 0x30) {
    picMasterCommand.Write(0x20);
    if (0x28 <= interruptNumber) {
      picSlaveCommand.Write(0x20);
    }
  }


  return esp;
}
