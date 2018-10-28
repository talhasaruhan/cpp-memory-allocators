#pragma once

#include "Allocator.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory>

class SystemAllocator : public Allocator {
public:
    SystemAllocator();
    ~SystemAllocator();

    void* Alloc(size_t sz, size_t alignment) final;
    inline void Free(void*&) final;
    inline void Release() final;
    inline void Reset() final;
    inline void ZeroMem() final;
    inline void Layout() final;
};
