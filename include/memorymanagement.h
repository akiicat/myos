
#ifndef __MYOS__MEMORYMANAGEMENT_H
#define __MYOS__MEMORYMANAGEMENT_H

#include <common/types.h>

namespace myos {

  // double link list
  struct MemoryChunk {
    MemoryChunk* next;
    MemoryChunk* prev;
    bool allocated;
    common::size_t size;
  };

  class MemoryManager {
    protected:
      MemoryChunk* first;

    public:
      // we want to call the allocation later from static functions
      static MemoryManager *activeMemoryManager;

      MemoryManager(common::size_t start, common::size_t size);
      ~MemoryManager();

      void* malloc(common::size_t size);
      void free(void* ptr);
  };

}

// self define new operator if you want to use malloc
void* operator new(unsigned size);
void* operator new[](unsigned size);

// placement new operator
// when you want to call a constructor explicitly on REM
// that you have already allocated in any way
void* operator new(unsigned size, void* ptr);
void* operator new[](unsigned size, void* ptr);

void operator delete(void* ptr);
void operator delete[](void* ptr);

#endif

