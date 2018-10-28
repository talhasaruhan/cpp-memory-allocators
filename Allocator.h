#pragma once

#include "VMAllocator.h"
#include <iostream>
#include <stdint.h>

struct ALLOC_PATTERN_FIRST_FIT {};
struct ALLOC_PATTERN_BEST_FIT {};

struct ALLOC_BUFFER_STATIC {};
struct ALLOC_BUFFER_STATIC_PREALLOC {};
struct ALLOC_BUFFER_VMDYNAMIC {};

enum BYTE_PREFIX {
    KB = 1024,
    MB = 1024*1024,
    GB = 1024*1024*1024
};

class Allocator {
public:
    Allocator();
    ~Allocator();

    // allocate
    virtual void* Alloc(size_t sz, size_t alignment) =0;
    
    // deallocate
    virtual void Free(void*&) =0;
    
    // reset allocator state
    virtual void Reset() =0;
    
    // override memory region with zeroes
    virtual void ZeroMem() =0;
    
    // release (if exists) the buffer allocated from the OS at the initialiation of the allocator.
    virtual void Release() = 0;

    virtual void Layout() =0;
};
