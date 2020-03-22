.set IRQ_BASE, 0x20

.section .text

.extern _ZN4myos21hardwarecommunication16InterruptManager15HandleInterruptEhj

# interrupt service routines
.macro HandleException num
.global _ZN4myos21hardwarecommunication16InterruptManager19HandleException\num\()Ev
_ZN4myos21hardwarecommunication16InterruptManager19HandleException\num\()Ev:
  movb $\num, (interruptnumber)
  jmp int_bottom
.endm

.macro HandleInterruptRequest num
.global _ZN4myos21hardwarecommunication16InterruptManager26HandleInterruptRequest\num\()Ev
_ZN4myos21hardwarecommunication16InterruptManager26HandleInterruptRequest\num\()Ev:
  movb $\num + IRQ_BASE, (interruptnumber)

  # in the handler interrupt request we need to just push something like 0
  # because this is for the error flag of CPUState
  # so otherwise we would have just in the interrupt request the thing would get confused
  # so for an exception the CPU pushes this error value
  # and for an interrupt request it doesn't push anything
  # so we have to push something so that this structure works
  pushl $0

  jmp int_bottom
.endm

HandleException 0x00
HandleException 0x01
HandleException 0x02
HandleException 0x03
HandleException 0x04
HandleException 0x05
HandleException 0x06
HandleException 0x07
HandleException 0x08
HandleException 0x09
HandleException 0x0A
HandleException 0x0B
HandleException 0x0C
HandleException 0x0D
HandleException 0x0E
HandleException 0x0F
HandleException 0x10
HandleException 0x11
HandleException 0x12
HandleException 0x13

HandleInterruptRequest 0x00 # 0x20 timer
HandleInterruptRequest 0x01 # 0x21 keyboard
HandleInterruptRequest 0x02
HandleInterruptRequest 0x03
HandleInterruptRequest 0x04
HandleInterruptRequest 0x05
HandleInterruptRequest 0x06
HandleInterruptRequest 0x07
HandleInterruptRequest 0x08
HandleInterruptRequest 0x09
HandleInterruptRequest 0x0A
HandleInterruptRequest 0x0B
HandleInterruptRequest 0x0C # 0x2C mouse
HandleInterruptRequest 0x0D
HandleInterruptRequest 0x0E
HandleInterruptRequest 0x0F
HandleInterruptRequest 0x31

HandleInterruptRequest 0x80 # syscall

int_bottom:

  # we just push these values manually here
  # and everything down the multitasking is pushed by the processor or by ourselves up there

  # save registers
  # before we jump into the handle interrupt function.
  # we actually have to store away the old values of the registers
  # pusha       # push all
  # pushl %ds   # data segment
  # pushl %es   # extra segment
  # pushl %fs   # frame pointer
  # pushl %gs   # be used to manage thread local storage

  # we push them in the other order of CPUState
  pushl %ebp
  pushl %edi
  pushl %esi

  pushl %edx
  pushl %ecx
  pushl %ebx
  pushl %eax

  # load ring 0 segment register
  # cld
  # mov $0x10, %eax
  # mov %eax, %eds
  # mov %eax, %ees


  # call C++ Handler
  # basically jump into the handle interrupt function
  pushl %esp
  push (interruptnumber)
  call _ZN4myos21hardwarecommunication16InterruptManager15HandleInterruptEhj

  # add %esp, 6
  # add $5, %esp   # pop the old stack pointer
  mov %eax, %esp # switch the stack # overwrite esp with the result value from the handleInterrupt function

  # restore registers
  popl %eax
  popl %ebx
  popl %ecx
  popl %edx

  popl %esi
  popl %edi
  popl %ebp
  # popl %gs
  # popl %fs
  # popl %es
  # popl %ds
  # popa

  # for this error value that we have just pushed up there
  # we need to add 4 to ESP so here we pop that again
  add $4, %esp

.global _ZN4myos21hardwarecommunication16InterruptManager15InterruptIgnoreEv

_ZN4myos21hardwarecommunication16InterruptManager15InterruptIgnoreEv:
  # at the end of this, we need to tell the porcess okay
  # we are finished handling the interrupts so you can return to what you were doing before
  # or if we have switched the stack then it will jump it will work on the diffreent process
  iret       # interrupt return

# we need to also have this interrupt number is a byte which is initialized to zero
.data
  interruptnumber: .byte 0

