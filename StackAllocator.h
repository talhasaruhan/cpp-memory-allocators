#pragma once

#include "Allocator.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory>

class StackAllocator : public Allocator {
public:
    StackAllocator(size_t sz);
    StackAllocator(void* buffer, size_t sz);
    ~StackAllocator();

    void* Alloc(size_t sz, size_t alignment) final;
    inline void Free(void*&) final;
    inline void Release() final;
    inline void Reset() final;
    inline void ZeroMem() final;
    inline void Layout() final;

protected:

    struct AllocHeader {
        uint8_t padding;
    };

private:
    void* m_start;
    void* m_cur;
    void* m_end;
    size_t m_size;
    size_t m_free;
    bool m_initialized;
    bool m_preAlloc;
};
