/*
 * 
 * We have a kernel which we can boot and it is able to do a
 * lot of stuff. It can load from a hard drive. It can read
 * and write from to the network. It can allocate memory.
 * It can print to the screen. It can talk to the PCI Hardware,
 * read from the mouse, and so on.
 *
 * The kernel can do is it can allocate some RAM and then it
 * can load something from a hard drive in there, for example
 * an executable binary file, and then it can attach executable
 * binary file to its task management, so then it would execute
 * this binary file in parallel.
 *
 *                   +--------+       RAM
 *    HD ------------|        |     --------+-----+-------
 *   Net ------------| Kernel |             | bin |
 * Print ------------|        |     --------+-----+-------
 *                   +--------+
 *
 * In the long run, you want to disallow certain things that
 * binary file is not supposed to talk to the hard drive directly.
 * Because you want something like a user mode and the programs
 * that run in this user mode are not supposed to write to certain
 * system directories. But if you allow the program to call the
 * assembler commands `outb`, then it would be able to talk
 * directly to the hard drive.
 *
 * So how user space works is tell the processor "I'm going into
 * user space now". If there is an `outb` command, just ignore it
 * or I generated a general protection fault, something like a blue
 * screen, something different, or tell me that this has happened
 * and then I will just kill this process.
 *
 * you have disallowed this binary from talking to the hard drive
 * directly but programs do read files (not system files). There
 * has to be some way for the binary to talk to the hard drive but
 * not through the `outb` command. Because if you allowed the
 * `outb` command, you couldn't stop this binary from writing to
 * system files. So what you do instead is called a system call,
 * so the binary basically call a software interrupt (0x80)
 *
 * Before that you set the eax register and ebx register to something,
 * eax usually will be the code of an instruction:
 *
 * | Name                | eax  | ebx                         | ecx                    | edx          | esi | edi |
 * | ------------------- | ---- | --------------------------- | ---------------------- | ------------ | --- | --- |
 * | sys_restart_syscall | 0x00 | -                           | -                      | -            | -   | -   |
 * | sys_exit            | 0x01 | int error_code              | -                      | -            | -   | -   |
 * | sys_fork            | 0x02 | struct pt_regs*             | -                      | -            | -   | -   |
 * | sys_read            | 0x03 | unsigned int fd             | char __user *buf       | size_t count | -   | -   |
 * | sys_write           | 0x04 | unsigned int fd             | const char __user *buf | size_t count | -   | -   |
 * | sys_open            | 0x05 | const char __user *filename | int flags              | int mode     | -   | -   |
 *
 * From: https://syscalls.kernelgrok.com/
 *
 * Here you can see a lot of these codes. On the left, for example,
 * if you write a `5` into eax it means that ebx is a pointer to a
 * file name and I want to open that file. So if you set eax to 5
 * and ebx to the string, "/foo.txt", that means I want to open
 * this file
 *
 *   BX = "/foo.txt"
 *   AX = 5
 *
 *   Interrupt = 0x80
 *
 * Now on the Kernel site you just have to attach an interrupt
 * handler which handles 0x80 interrupts and which then looks
 * at AX and BX registers and says ok. He called the system
 * called number 5, so he wants to open this file and now
 * the Kernel can look at BX register file name "/foo.txt" and
 * decide slash is a system directory I will not allow this.
 * And it might set eax to some error code and just return.
 * But if you try to open something that you are allowed to open,
 * you can of course look at the process ID and in this
 * multitasking (class for `Task` in multitasking.h). you could
 * have a user ID who executes this process in there and then
 * the kernel could look at this user ID and say this user wait
 * this is a root user so he is allowed to open this. Then I
 * don't set eax to an error code. (it might set eax to 0 and
 * ebx to a file handle or something)
 *
 * The interesting part is the way this binary file can now
 * communicate with the kernel. The binary file sets the
 * registers called an interrupt and then the interrupt handler
 * looks at these (eax, ebx) registers and says "Is this user
 * allowed to do this?". If yes, then I (kernel) do what he (binary file)
 * asks me to do and otherwise I will not do what he asked me
 * to do and give him some error code or terminate the program
 *
 */

#ifndef __MYOS__SYSCALLS_H
#define __MYOS__SYSCALLS_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>

namespace myos {

  class SyscallHandler : public hardwarecommunication::InterruptHandler {

    public:
      SyscallHandler(hardwarecommunication::InterruptManager* interruptManager, common::uint8_t interruptNumber);
      ~SyscallHandler();

      virtual common::uint32_t HandleInterrupt(common::uint32_t esp);
  };

}

#endif
