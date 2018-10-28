#pragma once

#include "Allocator.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <memory>

class LinearAllocator : public Allocator {
public:
    LinearAllocator(size_t sz);
    LinearAllocator(void* buffer, size_t sz);
    ~LinearAllocator();

    void* Alloc(size_t sz, size_t alignment) final;
    inline void Free(void*&) final;
    inline void Release() final;
    inline void Reset() final;
    inline void ZeroMem() final;
    inline void Layout() final;

private:
    void* m_start;
    void* m_cur;
    void* m_end;
    size_t m_size;
    size_t m_free;
    bool m_initialized;
    bool m_preAlloc;
};
