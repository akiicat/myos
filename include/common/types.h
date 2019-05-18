
#ifndef __MYOS__COMMON__TYPES_H
#define __MYOS__COMMON__TYPES_H

namespace myos {

  namespace common {

    typedef char int8_t;
    typedef unsigned char uint8_t;

    typedef short int16_t;
    typedef unsigned short uint16_t;

    typedef int int32_t;
    typedef unsigned int uint32_t;

    typedef long long int int64_t;
    typedef unsigned long long int uint64_t;

    typedef const char* string;

    // on a 32-bit system the memory address are 32-bit
    // and that is why we set size_t to uint32 here
    // on a 64-bit system we would set this to uint64 instead
    // because then we could dereference 64-bit integers
    typedef uint32_t size_t;
  }

}

#endif

