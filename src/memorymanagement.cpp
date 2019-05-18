#include <memorymanagement.h>

using namespace myos;
using namespace myos::common;

MemoryManager* MemoryManager::activeMemoryManager = 0;

MemoryManager::MemoryManager(size_t start, size_t size) {
  // set the activeMemoryManager to this
  activeMemoryManager = this;

  // the data that the MemoryManager is going to handle
  // so this is where we put the first MemoryChunk
  // but if the size that we get here **isn't sufficient**
  // we should really protect ourselves against the situation
  // because if the situation arises
  // and we would be writing outside of the area that we are allowed to write
  // so it's not really likely that this happen but to make sure
  if (size < sizeof(MemoryChunk)) {
    first = 0;
  }
  else {

    first = (MemoryChunk*)start;

    // first chunk is not allocated
    first->allocated = false;

    // set the first prev and next pointer are 0
    first->prev = 0;
    first->next = 0;

    first->size = size - sizeof(MemoryChunk);
  }
}

MemoryManager::~MemoryManager() {
  if (activeMemoryManager == this) {
    activeMemoryManager = 0;
  }
}

void* MemoryManager::malloc(size_t size) {

  // it doest that in a relatively slow way
  // you know if we allocate a million bytes one by one then the next allocation will take a million iterations to find some free space
  //
  // iterate through the list of chunks and look for a chunk that is large enough
  MemoryChunk *result = 0;
  for (MemoryChunk *chunk = first; chunk != 0; chunk = chunk->next) {
    // chunk is free and large enough
    if (chunk->size > size && !chunk->allocated) {
      result = chunk;
      break;
    }
  }

  // if we haven't found anything result large enough
  if (result == 0) {
    return 0;
  }

  // 
  // the chunk that is free and large enough
  // 
  // +----------------+--------------------------------------------------------------------+
  // |  result chunk  |           result size                                              |
  // +----------------+--------------------------------------------------------------------+
  // 
  // if the result is larger than what we need then we split it
  // 
  //                 +------------- prev <--------------+              +---------- prev <---
  //                 v                                  |              v
  // +-------------------------------------------------------------------------------------+
  // |  result chunk  |        requested size          |    new chunk   |  new chunk size  |
  // +-------------------------------------------------------------------------------------+
  //                 |                                  ^              |
  //                 +------------> next ---------------+              +---------> next ----
  // 
  if (result->size >= size + sizeof(MemoryChunk) + 1) {
    // make a new chunk ane we put that after to the result
    MemoryChunk *temp = (MemoryChunk*)((size_t)result + size + sizeof(MemoryChunk));

    temp->allocated = false;

    // calculate the new chunk size: just cut off the new chunk and the size of requested
    temp->size = result->size - size - sizeof(MemoryChunk);
    result->size = size;

    // modify pointer
    temp->prev = result;
    temp->next = result->next;
    result->next = temp;

    // if there is something, set previous point must be set to temp
    if (temp->next != 0) {
      temp->next->prev = temp;
    }
  }

  // result size is not larger than the requested size + the size of a new memory chunk + one byte of new size
  // 
  // +----------------+----------------------------+---+
  // |  result chunk  |        requested size      | X |
  // +----------------+----------------------------+---+
  // 
  // we cannot split it, so the program that requested the data will jsut have a few bytes left over and wasted
  // 
  // +----------------+--------------------------------+
  // |  result chunk  |        requested size          |
  // +----------------+--------------------------------+
  // 
  // so we mark the few bytes as allocated
  result->allocated = true;

  // return the result address + the size of MemoryChunk
  // so this is a pointer to the beginning of the region that I have allocated
  //
  // +-------------------------------------------------+
  // |  result chunk  |        requested size          |
  // +----------------+--------------------------------+
  //                  ^
  //          return this address
  return (void*)(((size_t)result) + sizeof(MemoryChunk));
}

void MemoryManager::free(void* ptr) {
  // get the chunk pointer by subtract the size of MemoryChunk
  //
  //   +----------------+--------------------------------+
  //   |      chunk     |          chunk size            |
  //   +----------------+--------------------------------+
  //   ^                ^
  // chunk             ptr
  MemoryChunk *chunk = (MemoryChunk*)((size_t)ptr - sizeof(MemoryChunk));

  chunk->allocated = false;

  // merge the chunk with the previous one
  if (chunk->prev != 0 && !chunk->prev->allocated) {
    // set the next pointer of the previous one to the next of the chunk
    chunk->prev->next = chunk->next;
    chunk->prev->size += chunk->size + sizeof(MemoryChunk);

    // if there is a next one
    // update the previous link of the next chunk
    if (chunk->next != 0) {
      chunk->next->prev = chunk->prev;
    }

    chunk = chunk->prev;
  }

  // swallow the next chunk
  if (chunk->next != 0 && !chunk->next->allocated) {
    chunk->size += chunk->next->size + sizeof(MemoryChunk);
    chunk->next = chunk->next->next;

    // if the new next chunk exists then we set it its previous pointer to ourselves
    if (chunk->next != 0) {
      chunk->next->prev = chunk;
    }
  }
}

