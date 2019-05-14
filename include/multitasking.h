#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos {

  struct CPUState {
    common::uint32_t eax; // accumulator register
    common::uint32_t ebx; // base register
    common::uint32_t ecx; // counting register
    common::uint32_t edx; // data register

    common::uint32_t esi; // stack index
    common::uint32_t edi; // data index
    common::uint32_t ebp; // stack base pointer

    // elements that we have been pushing so far already
    common::uint32_t gs;
    common::uint32_t fs;
    common::uint32_t es;
    common::uint32_t ds;

    // one integer for an error code
    // go into what that is for
    common::uint32_t error; // error code

    // the values down here are being pushed by the processor
    // and everything up here is what we push in the interrupt stubs
    common::uint32_t eip; // instruction pointer
    common::uint32_t cs; // the code segment instruction pointer
    common::uint32_t eflags; 
    common::uint32_t esp; // stack pointer
    common::uint32_t ss; // stack segment
  } __attribute__((packed));

  class Task {
    // the TaskManager might have to work inside the values of the Task
    // so make TaskManager a friend
    friend class TaskManager;

    private:
      common::uint8_t stack[4096]; // 4 KiB

      // I will have a pointer to the head after to the top element of the task stack
      // but I would put a data structure over the task stack areas
      // with a CPU pushed and user pushed data
      //
      // I'm going to call this CPU state
      // so the pointer to the top is actually a pointer to a CPU state
      // because the data that you find there is the state of the CPU
      CPUState* cpustate;

    public:
      // in the constructor the task will have to talk to the GlobalDescriptorTable
      // and it needs a function pointer to the function that is supposed to be executed
      Task(GlobalDescriptorTable* gdt, void entrypoint());
      ~Task();



  };

  class TaskManager {
    private:
      // the TaskManager will basically have an array of these tasks
      Task* tasks[256];

      // the number of tasks in this array
      int numTasks;

      // and the index of the currently active task
      //
      // we push this ESP and the pointer to the task stack is overwritten after the handle interrupt is finished
      // so we need to store that somewhere because otherwise we couldn't go back to executing that task ever
      int currentTask;

    public:
      TaskManager();
      ~TaskManager();

      bool AddTask(Task* task);

      // Here we will have a method which does the scheduling
      // we will just use round-robin scheduling
      // so just have a linear array of task pointers
      // and then every time one task is executing
      // and we go into this timer interrupt
      // and we will go to the next task... and start over at the beginning
      CPUState* Schedule(CPUState* cpustate);

  };

}

#endif
