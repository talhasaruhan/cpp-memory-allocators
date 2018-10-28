#pragma once

#include "Allocator.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <memory>

class PoolAllocator : public Allocator {
public:
    PoolAllocator(size_t sz, size_t pgSz);
    PoolAllocator(void* buffer, size_t sz, size_t pgSz);
    ~PoolAllocator();

    void* Alloc(size_t sz, size_t alignment) final;
    inline void Free(void*&) final;
    inline void Release() final;
    inline void Reset() final;
    inline void ZeroMem() final;
    inline void Layout() final;

protected:
    struct FreePageHeader {
        void* ptr;
    };

private:
    void buildLinkedList();

    void* m_start;
    void* m_end;
    void* m_llStart;
    size_t m_size;
    size_t m_pgSize;
    bool m_initialized;
    bool m_preAlloc;
};
