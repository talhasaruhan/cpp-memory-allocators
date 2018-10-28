#pragma once

#include "Allocator.h"
#include "VMLinearAllocator.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <memory>

/*
While this allocator is designed for arbitrary allocations,
It's not meant to replace system malloc, as it is not as performant
nor it has general usability in mind.

Use cases for this allocator include:
Manager for sub-allocators (only a few allocations are made
and they can be of different sizes)
When you need to constrain the range on which your general
purpose allocator operates on.

Note:
2-6X faster allocations (unless a new page is committed)
2-10X faster free in a LIFO or FIFO order
10-20X slower free in a random order
*/

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
class SequentialListAllocator : public Allocator {

public:
    // STATIC & VMDYNAMIC
    SequentialListAllocator(size_t sz);

    // STATIC PREALLOC
    SequentialListAllocator(void* buffer, size_t sz);

    ~SequentialListAllocator();

    void* Alloc(size_t sz, size_t alignment) final;

    void Free(void*&) final;
    inline void Release() final;
    inline void Reset() final;
    inline void ZeroMem() final;

    void Layout() final;
    
protected:
    struct FreeBlockHeader {
        void* next;
        void* prev;
        size_t sz;
    };

    struct AllocatedBlockHeader {
        size_t sz;
        size_t padding;
    };

private:
    /* FUNCTIONS */

    template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
    constexpr inline void* ALLOC(size_t sz, size_t alignment) {
        if constexpr(std::is_same<_ALLOC_BUFFER, ALLOC_BUFFER_VMDYNAMIC>::value) {
            if constexpr(std::is_same<_ALLOC_PATTERN, ALLOC_PATTERN_FIRST_FIT>::value) {
                return alloc_VMDYNAMIC_FIRST_FIT(sz, alignment);
            }
            else if constexpr(std::is_same<_ALLOC_PATTERN, ALLOC_PATTERN_BEST_FIT>::value) {
                return alloc_VMDYNAMIC_BEST_FIT(sz, alignment);
            }
        }
        else {
            if constexpr(std::is_same<_ALLOC_PATTERN, ALLOC_PATTERN_FIRST_FIT>::value) {
                return alloc_STATIC_FIRST_FIT(sz, alignment);
            }
            else if constexpr(std::is_same<_ALLOC_PATTERN, ALLOC_PATTERN_BEST_FIT>::value) {
                return alloc_STATIC_BEST_FIT(sz, alignment);
            }
        }
    }

    void* alloc_STATIC_FIRST_FIT(size_t sz, size_t alignment);
    void* alloc_STATIC_BEST_FIT(size_t sz, size_t alignment);
    void* alloc_VMDYNAMIC_FIRST_FIT(size_t sz, size_t alignment);
    void* alloc_VMDYNAMIC_BEST_FIT(size_t sz, size_t alignment);

    void* alloc_VMDYNAMIC_VMEXPAND(size_t sz, size_t alignment);

    inline void _fitPadding(ptrdiff_t& padding, size_t alignment);
    inline size_t _splitBlock(void* block, void* prevBlock, void* ptr, size_t sz);
    inline void* _fitToBlock(void* block, size_t sz, size_t alignment, ptrdiff_t& leftover, ptrdiff_t& padding);

    inline void build();
    
    /* VARIABLES */

    size_t m_size;

    void* m_llStart;
    void* m_llEnd;
    void* m_start;
    void* m_end;

    bool m_initialized;

    VMLinearAllocator* m_vmAllocator;
    uint32_t m_nVMPages;

    /* CONSTEXPRS */

    static constexpr size_t RESERVE_VIRTUAL_ADDRESS_SPACE = 1024 * 1024 * 1024;
    static constexpr size_t allocHeaderSize = sizeof(AllocatedBlockHeader);
    static constexpr size_t freeHeaderSize = sizeof(FreeBlockHeader);
};