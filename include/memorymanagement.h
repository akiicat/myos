
#ifndef __MYOS__MEMORYMANAGEMENT_H
#define __MYOS__MEMORYMANAGEMENT_H

#include <common/types.h>
#include <gdt.h>

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

#endif

