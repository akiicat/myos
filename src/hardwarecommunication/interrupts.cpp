#include <hardwarecommunication/interrupts.h>

using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;

// forward definition of printf
void printf(char*);
void printfHex(uint8_t);

InterruptHandler::InterruptHandler(InterruptManager* interruptManager, uint8_t InterruptNumber) {
  this->InterruptNumber = InterruptNumber;
  this->interruptManager = interruptManager;
  interruptManager->handlers[InterruptNumber] = this;
}

InterruptHandler::~InterruptHandler() {
  if (interruptManager->handlers[InterruptNumber] == this) {
    interruptManager->handlers[InterruptNumber] = 0;
  }
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp) {
  return esp;
}

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
    uint8_t interrupt,
    uint16_t CodeSegment,
    void (*handler)(),
    uint8_t DescriptorPrivilegeLevel,
    uint8_t DescriptorType)
{
  // address of pointer to code segment (relative to global descriptor table)
  // and address of the handler (relative to segment)
  interruptDescriptorTable[interrupt].handlerAddressLowBits = ((uint32_t) handler) & 0xFFFF;
  interruptDescriptorTable[interrupt].handlerAddressHighBits = (((uint32_t) handler) >> 16) & 0xFFFF;
  interruptDescriptorTable[interrupt].gdt_codeSegmentSelector = CodeSegment;

  const uint8_t IDT_DESC_PRESENT = 0x80;
  // access type is 0x8E for ignore interrupt request
  interruptDescriptorTable[interrupt].access = IDT_DESC_PRESENT | ((DescriptorPrivilegeLevel & 3) << 5) | DescriptorType;
  interruptDescriptorTable[interrupt].reserved = 0;
}
InterruptManager::InterruptManager(uint16_t hardwareInterruptOffset, GlobalDescriptorTable* globalDescriptorTable, TaskManager* taskManager)
: programmableInterruptControllerMasterCommandPort(0x20),
  programmableInterruptControllerMasterDataPort(0x21),
  programmableInterruptControllerSlaveCommandPort(0xA0),
  programmableInterruptControllerSlaveDataPort(0xA1)
{
  this->taskManager = taskManager;

  // the interrupt knows the taskManager
  this->hardwareInterruptOffset = hardwareInterruptOffset;

  // Get GDT code segment
  uint32_t CodeSegment = globalDescriptorTable->CodeSegmentSelector();
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
  for (uint8_t i = 255; i > 0; --i) {
    SetInterruptDescriptorTableEntry(i, CodeSegment, &InterruptIgnore, 0, IDT_INTERRUPT_GATE);
    handlers[i] = 0;
  }
  SetInterruptDescriptorTableEntry(0, CodeSegment, &InterruptIgnore, 0, IDT_INTERRUPT_GATE);
  handlers[0] = 0;

  // The first Programmable Interrupt Controller (PIC)

  SetInterruptDescriptorTableEntry(0x00, CodeSegment, &HandleException0x00, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x01, CodeSegment, &HandleException0x01, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x02, CodeSegment, &HandleException0x02, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x03, CodeSegment, &HandleException0x03, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x04, CodeSegment, &HandleException0x04, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x05, CodeSegment, &HandleException0x05, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x06, CodeSegment, &HandleException0x06, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x07, CodeSegment, &HandleException0x07, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x08, CodeSegment, &HandleException0x08, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x09, CodeSegment, &HandleException0x09, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x0A, CodeSegment, &HandleException0x0A, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x0B, CodeSegment, &HandleException0x0B, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x0C, CodeSegment, &HandleException0x0C, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x0D, CodeSegment, &HandleException0x0D, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x0E, CodeSegment, &HandleException0x0E, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x0F, CodeSegment, &HandleException0x0F, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x10, CodeSegment, &HandleException0x10, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x11, CodeSegment, &HandleException0x11, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x12, CodeSegment, &HandleException0x12, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(0x13, CodeSegment, &HandleException0x13, 0, IDT_INTERRUPT_GATE);

  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x00, CodeSegment, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE); // timer
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x01, CodeSegment, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE); // keyboard
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x02, CodeSegment, &HandleInterruptRequest0x02, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x03, CodeSegment, &HandleInterruptRequest0x03, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x04, CodeSegment, &HandleInterruptRequest0x04, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x05, CodeSegment, &HandleInterruptRequest0x05, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x06, CodeSegment, &HandleInterruptRequest0x06, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x07, CodeSegment, &HandleInterruptRequest0x07, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x08, CodeSegment, &HandleInterruptRequest0x08, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x09, CodeSegment, &HandleInterruptRequest0x09, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0A, CodeSegment, &HandleInterruptRequest0x0A, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0B, CodeSegment, &HandleInterruptRequest0x0B, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0C, CodeSegment, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE); // mouse
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0D, CodeSegment, &HandleInterruptRequest0x0D, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0E, CodeSegment, &HandleInterruptRequest0x0E, 0, IDT_INTERRUPT_GATE);
  SetInterruptDescriptorTableEntry(hardwareInterruptOffset + 0x0F, CodeSegment, &HandleInterruptRequest0x0F, 0, IDT_INTERRUPT_GATE);

  SetInterruptDescriptorTableEntry(                          0x80, CodeSegment, &HandleInterruptRequest0x80, 0, IDT_INTERRUPT_GATE); // syscall

  programmableInterruptControllerMasterCommandPort.Write(0x11);
  programmableInterruptControllerSlaveCommandPort.Write(0x11);

  // remap
  programmableInterruptControllerMasterDataPort.Write(hardwareInterruptOffset);
  programmableInterruptControllerSlaveDataPort.Write(hardwareInterruptOffset+8);

  programmableInterruptControllerMasterDataPort.Write(0x04);
  programmableInterruptControllerSlaveDataPort.Write(0x02);

  programmableInterruptControllerMasterDataPort.Write(0x01);
  programmableInterruptControllerSlaveDataPort.Write(0x01);

  programmableInterruptControllerMasterDataPort.Write(0x00);
  programmableInterruptControllerSlaveDataPort.Write(0x00);

  // after creating the table, tell the processor to use it
  InterruptDescriptorTablePointer idt_pointer;
  idt_pointer.size  = 256 * sizeof(GateDescriptor) - 1;    // IDT size
  idt_pointer.base  = (uint32_t)interruptDescriptorTable;  // pointer to IDT

  // tell the processor to load IDT
  asm volatile("lidt %0" : : "m" (idt_pointer)); // lidt: load IDT
}

InterruptManager::~InterruptManager() {
  Deactivate();
}

uint16_t InterruptManager::HardwareInterruptOffset() {
  return hardwareInterruptOffset;
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
    asm("cli");
  }
}

uint32_t InterruptManager::HandleInterrupt(uint8_t interrupt, uint32_t esp) {

  if (ActiveInterruptManager != 0) {
    return ActiveInterruptManager->DoHandleInterrupt(interrupt, esp);
  }

  // return the same stack pointer for disable task switching
  // since we don't have multiple processes
  return esp;
}

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interrupt, uint32_t esp) {
  if (handlers[interrupt] != 0) {
    esp = handlers[interrupt]->HandleInterrupt(esp);
  }
  else if (interrupt != hardwareInterruptOffset) {
    printf("UNHANDLED INTERRUPT 0x");
    printfHex(interrupt);
  }

  // setting ESP again so we could actually remove that from the interrupt handlers
  // but I will leave it there right now
  if (interrupt == hardwareInterruptOffset) {
    // so if we have a timer interrupt
    // then I will set ESP to the taskManager schedule
    //
    // you could fix the data types here now so that the uint32 becomes CPUState
    // I think that would be a little bit more clean
    esp = (uint32_t)taskManager->Schedule((CPUState*)esp);
  }

  // hardware interrupts must be acknowledged
  if (hardwareInterruptOffset <= interrupt && interrupt < hardwareInterruptOffset+16) {
    programmableInterruptControllerMasterCommandPort.Write(0x20);
    if (hardwareInterruptOffset + 8 <= interrupt) {
      programmableInterruptControllerSlaveCommandPort.Write(0x20);
    }
  }

  return esp;
}
