#include <syscalls.h>

using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;

SyscallHandler::SyscallHandler(InterruptManager* interruptManager, myos::common::uint8_t interruptNumber)
  : InterruptHandler(interruptManager, interruptNumber + interruptManager->HardwareInterruptOffset()) {

}

SyscallHandler::~SyscallHandler() {

}

void printf(char* str);

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp) {
  CPUState* cpu = (CPUState*)esp;

  switch(cpu->eax) {
    case 4:
      printf((char*)cpu->ebx);
      break;
  }

  return esp;
}

