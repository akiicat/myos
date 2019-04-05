.set IRQ_BASE, 0x20

.section .text

.extern _ZN16InterruptManager15handleInterruptEhj
.global _ZN16InterruptManager22IgnoreInterruptRequestEv

# interrupt service routines
.macro HandleException num
.global _ZN16InterruptManager26HandleException\num\()Ev
_ZN16InterruptManager26HandleException\num\()Ev:
  movb $\num, (interruptnumber)
  jmp int_bottom
.endm

.macro HandleInterruptRequest num
.global _ZN16InterruptManager26HandleInterruptRequest\num\()Ev
_ZN16InterruptManager26HandleInterruptRequest\num\()Ev:
  movb $\num + IRQ_BASE, (interruptnumber)
  jmp int_bottom
.endm

HandleInterruptRequest 0x00   # timer
HandleInterruptRequest 0x01   # keyboard

int_bottom:

  # before we jump into the handle interrupt function.
  # we actually have to store away the old values of the registers
  pusha       # push all
  pushl %ds   # data segment
  pushl %es   # extra segment
  pushl %fs   # frame pointer
  pushl %gs   # be used to manage thread local storage

  # basically jump into the handle interrupt function
  pushl %esp
  push (interruptnumber)
  call _ZN16InterruptManager15handleInterruptEhj

  # add $5, %esp   # pop the old stack pointer
  movl %eax, %esp  # overwrite esp with the result value from the handleInterrupt function

  popl %gs
  popl %fs
  popl %es
  popl %ds
  popa

_ZN16InterruptManager22IgnoreInterruptRequestEv:
  # at the end of this, we need to tell the porcess okay
  # we are finished handling the interrupts so you can return to what you were doing before
  # or if we have switched the stack then it will jump it will work on the diffreent process
  iret       # interrupt return

# we need to also have this interrupt number is a byte which is initialized to zero
.data
  interruptnumber: .byte 0

