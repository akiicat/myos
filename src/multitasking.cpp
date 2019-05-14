#include <multitasking.h>
using namespace myos;
using namespace myos::common;
Task::Task(GlobalDescriptorTable* gdt, void entrypoint()) {
  // CPUState is supposed to be a pointer to the start of the task stack block here
  // and for a new task this is just all the way to the right
  // so we will set this just as a pointer to the stack
  //
  // the pointer of the start of the stack plus the size of the stack minus size of CPUState
  cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
  
  // set phony entries
  cpustate->eax = 0;
  cpustate->ebx = 0;
  cpustate->ecx = 0;
  cpustate->edx = 0;

  cpustate->esi = 0;
  cpustate->edi = 0;
  cpustate->ebp = 0;

  // cpustate->gs = 0;
  // cpustate->fs = 0;
  // cpustate->es = 0;
  // cpustate->ds = 0;

  // the error is popped anyway so we don't need to go at that
  //
  // I am not really sure this is a good idea
  // I mean this is the under the data segment 
  // I'll just comment that out
  // cpustate->error = 0;

  // instruction pointer is set to the entry point;
  cpustate->eip = (uint32_t) entrypoint;

  // in the tutorial, they just set this to 0x08,
  // but got a lot of general projections faults which is the equivalent of a blue screen
  //
  // then I set this to 0x10, from 8 to 16, and then it worked
  // but I don't think it's  a good idea to have a magic number here
  // because it's the value that you put there must really be the code segment selector from the global descriptor table
  //
  // the offset of the code segment I mean you can hard code this
  // but I really don't think it's a good idea
  // so taking this from global descriptor table is a much better idea
  cpustate->cs = gdt->CodeSegmentSelector();

  cpustate->eflags = 0x202;

  // comment this out because this only has to do with if we are dealing with user space and different security levels
  // and we are not doing that for now
  // cpustate->esp = 0;

  // stack segment also is also not used
  // cpustate->ss = 0;
}

Task::~Task() {
  
}

TaskManager::TaskManager() {
  // just set numTasks to 0 because we have no tasks in the beginning
  numTasks = 0;

  // and the current task to -1 because it's not set to anything legal inside the array
  currentTask = -1;
}

TaskManager::~TaskManager() {
  
}

bool TaskManager::AddTask(Task* task) {
  // if we already have 256 tasks in there then the array is full
  // so we just return false
  if (numTasks >= 256) {
    return false;
  }

  // otherwise we put the task in the next free spot and return true
  task[numTasks++] = task;

  return true;
}

CPUState* TaskManager::Schedule(CPUState* cpustate) {
  // if we don't have any tasks yet, we just return the old CPU state
  if (numTasks <= 0) {
    return cpustate;
  }

  // so if we are already doing the scheduling
  // then we store the old CPUState
  if (currentTask >= 0) {
    // store the old value
    // put the task back and to the list of tasks
    tasks[currentTask]->cpustate = cpustate;
  }

  // and here we just do the round-robin
  // if currentTask exceeds the size of the array
  // then we start over at the beginning
  if (++currentTask >= numTasks) {
    currentTask %= numTasks;
  }

  // and then we return the new currentTask
  return tasks[currentTask]->cpustate;
}

// notice if we really do have tasks
// the first time we run into this
// when currentTask is minus one
// then we don't store it away
// (for the next time)

