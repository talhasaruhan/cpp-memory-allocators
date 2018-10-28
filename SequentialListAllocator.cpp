#include "SequentialListAllocator.h"

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::SequentialListAllocator(size_t sz) :
    m_size(sz), m_initialized(false),
    m_vmAllocator(NULL), m_nVMPages(0)
{
    if constexpr(std::is_same<_ALLOC_BUFFER, ALLOC_BUFFER_STATIC>::value) {
        std::cout << "successfully constructed! STATIC\n";
        m_start = (uint8_t*)malloc(m_size * sizeof(uint8_t));
        m_end = (void*)((uintptr_t)m_start + m_size * sizeof(uint8_t));
        m_initialized = true;
        build();
    }
    else if constexpr(std::is_same<_ALLOC_BUFFER, ALLOC_BUFFER_VMDYNAMIC>::value) {
        std::cout << "successfully constructed! VMDYNAMIC\n";

        m_vmAllocator = new VMLinearAllocator(RESERVE_VIRTUAL_ADDRESS_SPACE);

        uint32_t vmPageSize = m_vmAllocator->PageSize();
        uint32_t n = (uint32_t)ceil((double)sz / vmPageSize);
        size_t vmAllocSize = (size_t)n * vmPageSize;

        m_start = m_vmAllocator->Alloc(n);
        m_end = (void*)((uintptr_t)m_start + vmAllocSize);
        m_size = vmAllocSize;
        m_nVMPages = n;
        m_initialized = true;
        build();
    }
    else {
        assert(false && "BUFFER WASN'T PROVIDED IN CTOR. TEMPLATE & ARGUMENT MISMATCH! MAKE SURE YOU \
            INSTANTIATE RIGHT CLASS AND CALL THE RIGHT CONSTRUCTOR.");
        return;
    }
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::SequentialListAllocator(void* buffer, size_t sz) :
    m_size(sz), m_initialized(true),
    m_vmAllocator(NULL), m_nVMPages(0)
{
    if constexpr(std::is_same<_ALLOC_BUFFER, ALLOC_BUFFER_STATIC_PREALLOC>::value) {
        std::cout << "successfully constructed! STATIC PREALLOC\n";
        m_start = buffer;
        m_end = (void*)((uintptr_t)m_start + m_size * sizeof(uint8_t));
        build();
    }
    else {
        assert(false && "BUFFER PROVIDED IN CTOR. TEMPLATE & ARGUMENT MISMATCH! MAKE SURE YOU \
            INSTANTIATE RIGHT CLASS AND CALL THE RIGHT CONSTRUCTOR.");
        return;
    }
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::~SequentialListAllocator() {
    if (m_initialized)
        Release();
    if (m_vmAllocator)
        delete m_vmAllocator;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::build() {
    FreeBlockHeader* header = new(m_start) FreeBlockHeader;

    header->sz = m_size;
    header->next = NULL;
    header->prev = NULL;

    m_llStart = m_start;
    m_llEnd = m_start;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void* SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::Alloc(size_t sz, size_t alignment) {
    assert((alignment & (alignment - 1)) == 0);

    return ALLOC<_ALLOC_BUFFER, _ALLOC_PATTERN>(sz, alignment);
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
size_t SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::_splitBlock(void* block, void* prevBlock, void* ptr, size_t sz) {
    FreeBlockHeader* blockHeader = (FreeBlockHeader*)block;

    ptrdiff_t remainingSpace = (uintptr_t)block + blockHeader->sz - (uintptr_t)ptr - sz;

    if (remainingSpace >= freeHeaderSize) {
        // create a new free block
        void* newBlock = (void*)((uintptr_t)ptr + sz);

        FreeBlockHeader* newFreeBlockHeader = new(newBlock) FreeBlockHeader;
        newFreeBlockHeader->sz = blockHeader->sz - ((uintptr_t)ptr - (uintptr_t)block + sz);
        newFreeBlockHeader->next = blockHeader->next;
        newFreeBlockHeader->prev = blockHeader->prev;

        if (prevBlock)
            ((FreeBlockHeader*)prevBlock)->next = newBlock;
        else
            m_llStart = newBlock;

        if (blockHeader->next)
            ((FreeBlockHeader*)((FreeBlockHeader*)blockHeader->next))->prev = newBlock;
        else
            m_llEnd = newBlock;

        return sz;
    }
    else {
        // cant create a new free block because remaining space is not large enough for header
        if (prevBlock)
            ((FreeBlockHeader*)prevBlock)->next = blockHeader->next;
        else
            m_llStart = blockHeader->next;

        if (blockHeader->next)
            ((FreeBlockHeader*)blockHeader->next)->prev = prevBlock;
        else
            m_llEnd = prevBlock;

        return (uintptr_t)blockHeader->sz - (uintptr_t)ptr + (uintptr_t)block;
    }

    return 0;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::_fitPadding(ptrdiff_t& padding, size_t alignment) {
    // if can't fit header into the padding
    if (padding < allocHeaderSize) {
        if ((allocHeaderSize - padding) % alignment == 0) {
            padding = allocHeaderSize;
        }
        else {
            padding += alignment * (1 + (allocHeaderSize - padding) / alignment);
        }
    }
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void* SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::_fitToBlock(
    void* block, size_t sz, size_t alignment, ptrdiff_t& leftover, ptrdiff_t& padding)
{
    void* ptr = (void*)(((uintptr_t)block + alignment - 1) & ~(alignment - 1));
    padding = (uintptr_t)ptr - (uintptr_t)block;
    _fitPadding(padding, alignment);
    ptr = (void*)((uintptr_t)block + padding);
    leftover = (ptrdiff_t)((FreeBlockHeader*)block)->sz - (padding + sz);
    return ptr;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void* SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::alloc_STATIC_FIRST_FIT(size_t sz, size_t alignment) {
    void* ptr = NULL;
    void* block = NULL;
    void* S_block = m_llStart;
    void* E_block = m_llEnd;

    ptrdiff_t leftover, padding;

    while (S_block && E_block && S_block <= E_block) {
        ptr = _fitToBlock(S_block, sz, alignment, leftover, padding);
        if (leftover >= 0) {
            block = S_block;
            break;
        }

        if (S_block != E_block) {
            ptr = _fitToBlock(E_block, sz, alignment, leftover, padding);
            if (leftover >= 0) {
                block = E_block;
                break;
            }
        }

        S_block = ((FreeBlockHeader*)S_block)->next;
        E_block = ((FreeBlockHeader*)E_block)->prev;
    }


    // for any LL that has more than 1 node, loop will terminate due to cmp, not 'cause any ptr is NULL
    if (!S_block || !E_block || S_block > E_block) {
        //std::cout << "Not enough free space!" << std::endl;
        return NULL;
    }


    AllocatedBlockHeader* allocHeader = new((void*)((uintptr_t)ptr - allocHeaderSize)) AllocatedBlockHeader;
    allocHeader->sz = _splitBlock(block, ((FreeBlockHeader*)block)->prev, ptr, sz);
    allocHeader->padding = padding;

    return ptr;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void* SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::alloc_STATIC_BEST_FIT(size_t sz, size_t alignment) {
    void* block = NULL;
    void* cache_block = NULL;

    void* ptr = NULL;
    void* cache_ptr = NULL;

    void* S_block = m_llStart;
    void* E_block = m_llEnd;

    ptrdiff_t leftover = 0, min_leftover = INT_MAX;
    ptrdiff_t padding, cache_padding;

    while (S_block && E_block && S_block <= E_block) {
        ptr = _fitToBlock(S_block, sz, alignment, leftover, padding);

        if (leftover >= 0 && leftover < min_leftover) {
            min_leftover = leftover;
            cache_block = S_block;
            cache_ptr = ptr;
            cache_padding = padding;
        }

        if (S_block != E_block) {
            ptr = _fitToBlock(E_block, sz, alignment, leftover, padding);

            if (leftover >= 0 && leftover < min_leftover) {
                min_leftover = leftover;
                cache_block = E_block;
                cache_ptr = ptr;
                cache_padding = padding;
            }
        }

        S_block = ((FreeBlockHeader*)S_block)->next;
        E_block = ((FreeBlockHeader*)E_block)->prev;
    }

    if (!cache_block) {
        //std::cout << "Not enough free space!" << std::endl;
        return NULL;
    }

    // split the current block
    AllocatedBlockHeader* allocHeader = new((void*)((uintptr_t)cache_ptr - allocHeaderSize)) AllocatedBlockHeader;
    allocHeader->sz = _splitBlock(cache_block, ((FreeBlockHeader*)cache_block)->prev, cache_ptr, sz);
    allocHeader->padding = cache_padding;

    return cache_ptr;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void* SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::alloc_VMDYNAMIC_FIRST_FIT(size_t sz, size_t alignment) {
    void* ptr = alloc_STATIC_FIRST_FIT(sz, alignment);
    if (ptr)
        return ptr;

    return alloc_VMDYNAMIC_VMEXPAND(sz, alignment);
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void* SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::alloc_VMDYNAMIC_BEST_FIT(size_t sz, size_t alignment) {
    void* ptr = alloc_STATIC_BEST_FIT(sz, alignment);
    if (ptr)
        return ptr;

    return alloc_VMDYNAMIC_VMEXPAND(sz, alignment);
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void* SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::alloc_VMDYNAMIC_VMEXPAND(size_t sz, size_t alignment) {
    // no free block was large enough
    // request new page

    uint32_t vmPageSize = m_vmAllocator->PageSize();
    uint32_t n;
    size_t vmAllocSize;
    void* block = NULL;
    void* prevBlock = NULL;
    void* ptr = NULL;
    ptrdiff_t leftover, padding;

    // set the start of the new block and calculate num of VM pages to commit
    if (m_llEnd && (uintptr_t)m_llEnd + ((FreeBlockHeader*)m_llEnd)->sz == (uintptr_t)m_end) {
        // last block is free (yet not big enough)
        block = m_llEnd;

        ptr = (void*)(((uintptr_t)block + alignment - 1) & ~(alignment - 1));
        padding = (uintptr_t)ptr - (uintptr_t)block;
        _fitPadding(padding, alignment);

        n = (int32_t)ceil((double)((uintptr_t)ptr + padding + sz - (uintptr_t)m_end) / vmPageSize);
        prevBlock = ((FreeBlockHeader*)m_llEnd)->prev;
    }
    else {
        // last block is occupied
        block = m_end;

        ptr = (void*)(((uintptr_t)block + alignment - 1) & ~(alignment - 1));
        padding = (uintptr_t)ptr - (uintptr_t)block;
        _fitPadding(padding, alignment);

        n = ceil((double)(sz + padding) / vmPageSize);
        prevBlock = m_llEnd;
        if (prevBlock) {
            ((FreeBlockHeader*)prevBlock)->next = block;
        }
    }

    // to be safe, allocate one more page than needed
    //n++;
    m_nVMPages += n;
    vmAllocSize = (size_t)n * vmPageSize;

    // every allocation is made from the end of the address space
    void* vmAlloc = m_vmAllocator->Alloc(n);
    assert(vmAlloc == m_end && "ERR VMALLOC IS NOT CONTIGUOUS");

    FreeBlockHeader* freeHeader = new(block) FreeBlockHeader;
    freeHeader->sz = (uintptr_t)m_end - (uintptr_t)block + vmAllocSize;
    freeHeader->next = NULL;
    freeHeader->prev = prevBlock;

    m_end = (void*)((uintptr_t)m_end + vmAllocSize);
    m_size += vmAllocSize;

    if (!m_llStart || !m_llEnd) {
        m_llStart = block;
        m_llEnd = block;
    }

    ptr = (void*)((uintptr_t)block + (uintptr_t)padding);

    AllocatedBlockHeader* allocHeader = new((void*)((uintptr_t)ptr - allocHeaderSize)) AllocatedBlockHeader;
    allocHeader->sz = _splitBlock(block, prevBlock, ptr, sz);
    allocHeader->padding = padding;

    return ptr;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::Free(void*& ptr) {
    if (!ptr)
        return;

    AllocatedBlockHeader* allocHeader = (AllocatedBlockHeader*)((uintptr_t)ptr - allocHeaderSize);
    size_t allocSize = allocHeader->sz;
    size_t allocPadding = allocHeader->padding;

    void* freeHeaderPtr = (void*)((uintptr_t)ptr - allocPadding);

    if (!m_llStart) {
        // no free blocks
        FreeBlockHeader* freeHeader = new(freeHeaderPtr) FreeBlockHeader;
        freeHeader->next = NULL;
        freeHeader->prev = NULL;
        freeHeader->sz = allocSize + allocPadding;
        m_llStart = freeHeaderPtr;
        m_llEnd = freeHeaderPtr;
    }
    else if (freeHeaderPtr < m_llStart) {
        // freed block will be the first element in the LL
        // new block at [ ptr-allocPadding, ptr+allocSize ]
        FreeBlockHeader* freeHeader = new(freeHeaderPtr) FreeBlockHeader;
        freeHeader->prev = NULL;

        if ((uintptr_t)m_llStart == (uintptr_t)ptr + allocSize) {
            // merge free blocks
            freeHeader->next = ((FreeBlockHeader*)m_llStart)->next;
            freeHeader->sz = allocSize + allocPadding + ((FreeBlockHeader*)m_llStart)->sz;
        }
        else {
            ((FreeBlockHeader*)m_llStart)->prev = freeHeaderPtr;
            freeHeader->next = m_llStart;
            freeHeader->sz = allocSize + allocPadding;
        }

        m_llStart = freeHeaderPtr;
    }
    else if (freeHeaderPtr > m_llEnd) {
        // last element in LL
        if ((uintptr_t)m_llEnd + ((FreeBlockHeader*)m_llEnd)->sz == (uintptr_t)freeHeaderPtr) {
            // merge if adjacent to the previously last block
            ((FreeBlockHeader*)m_llEnd)->next = NULL;
            ((FreeBlockHeader*)m_llEnd)->sz += allocSize + allocPadding;
        }
        else {
            FreeBlockHeader* freeHeader = new(freeHeaderPtr) FreeBlockHeader;
            ((FreeBlockHeader*)m_llEnd)->next = ptr;
            freeHeader->next = NULL;
            freeHeader->prev = m_llEnd;
            freeHeader->sz = allocSize + allocPadding;
            m_llEnd = freeHeaderPtr;
        }
    }
    else if (freeHeaderPtr > m_llStart && freeHeaderPtr < m_llEnd) {
        // freed block is in the middle of LL
        // iterate both from start and the end to find the proper place to 
        // append the allocated block to the LL

        void* S_block = ((FreeBlockHeader*)m_llStart)->next;
        void* S_prevBlock = m_llStart;

        void* E_block = ((FreeBlockHeader*)m_llEnd)->prev;
        void* E_prevBlock = m_llEnd;

        void* block = NULL;
        void* prevBlock = NULL;

        while (E_block && S_block) {

            if (S_block > freeHeaderPtr) {
                block = S_block;
                prevBlock = S_prevBlock;
                break;
            }

            if (E_block < freeHeaderPtr) {
                block = E_prevBlock;
                prevBlock = E_block;
                break;
            }

            if (S_block == freeHeaderPtr || E_block == freeHeaderPtr)
                return;

            S_prevBlock = S_block;
            S_block = ((FreeBlockHeader*)S_block)->next;

            E_prevBlock = E_block;
            E_block = ((FreeBlockHeader*)E_block)->prev;
        }

        bool isAdjacentToLeftFreeSpace = (prevBlock && (uintptr_t)(prevBlock)+((FreeBlockHeader*)prevBlock)->sz == (uintptr_t)freeHeaderPtr);
        bool isAdjacentToRightFreeSpace = (block && ((uintptr_t)ptr + allocSize == (uintptr_t)block));

        if (isAdjacentToRightFreeSpace && isAdjacentToLeftFreeSpace) {
            ((FreeBlockHeader*)prevBlock)->sz += allocSize + allocPadding + ((FreeBlockHeader*)block)->sz;
            ((FreeBlockHeader*)prevBlock)->next = ((FreeBlockHeader*)block)->next;
            if (((FreeBlockHeader*)block)->next)
                ((FreeBlockHeader*)(((FreeBlockHeader*)block)->next))->prev = prevBlock;
            if (block == m_llEnd)
                m_llEnd = prevBlock;
        }
        else if (isAdjacentToLeftFreeSpace) {
            ((FreeBlockHeader*)prevBlock)->sz += allocSize + allocPadding;
        }
        else if (isAdjacentToRightFreeSpace) {
            FreeBlockHeader* freeHeader = new(freeHeaderPtr) FreeBlockHeader;
            freeHeader->sz = allocSize + allocPadding + ((FreeBlockHeader*)block)->sz;
            freeHeader->next = ((FreeBlockHeader*)block)->next;
            freeHeader->prev = ((FreeBlockHeader*)block)->prev;
            ((FreeBlockHeader*)prevBlock)->next = freeHeaderPtr;
            if (((FreeBlockHeader*)block)->next)
                ((FreeBlockHeader*)(((FreeBlockHeader*)block)->next))->prev = freeHeaderPtr;
            if (block == m_llEnd)
                m_llEnd = freeHeaderPtr;
        }
        else {
            FreeBlockHeader* freeHeader = new(freeHeaderPtr) FreeBlockHeader;
            freeHeader->sz = allocSize + allocPadding;
            freeHeader->next = block;
            freeHeader->prev = prevBlock;
            ((FreeBlockHeader*)prevBlock)->next = freeHeaderPtr;
            ((FreeBlockHeader*)block)->prev = freeHeaderPtr;
        }
    }
    else {
        return;
    }
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::Layout() {
    void* block = m_llStart;

    std::cout << ((uintptr_t)m_end - (uintptr_t)m_start) << "\nNum of committed VM pages: " << m_nVMPages << " " << allocHeaderSize << "\nFree blocks at: ";

    while (block) {
        uintptr_t next = 0, prev = 0;
        if (((FreeBlockHeader*)block)->next != NULL)
            next = (uintptr_t)(((FreeBlockHeader*)block)->next) - (uintptr_t)m_start;

        if (((FreeBlockHeader*)block)->prev != NULL)
            prev = (uintptr_t)(((FreeBlockHeader*)block)->prev) - (uintptr_t)m_start;

        std::cout << " [" << prev << ", " << (uintptr_t)block - (uintptr_t)m_start << ", "
            << (uintptr_t)block - (uintptr_t)m_start + ((FreeBlockHeader*)block)->sz << ", "
            << next << "] ";
        block = ((FreeBlockHeader*)block)->next;
    }

    std::cout << std::endl;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::Release() {
    if constexpr(std::is_same<_ALLOC_BUFFER, ALLOC_BUFFER_STATIC_PREALLOC>::value) {
    }
    else if constexpr(std::is_same<_ALLOC_BUFFER, ALLOC_BUFFER_VMDYNAMIC>::value) {
        m_vmAllocator->Release();
    }
    else if constexpr(std::is_same<_ALLOC_BUFFER, ALLOC_BUFFER_STATIC>::value) {
        free(m_start);
    }
    m_initialized = false;
    return;
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::Reset() {
    ZeroMem();
    build();
}

template <typename _ALLOC_BUFFER, typename _ALLOC_PATTERN>
void SequentialListAllocator<_ALLOC_BUFFER, _ALLOC_PATTERN>::ZeroMem() {
    memset(m_start, 0, m_size);
}

template class SequentialListAllocator<ALLOC_BUFFER_STATIC_PREALLOC, ALLOC_PATTERN_FIRST_FIT>;
template class SequentialListAllocator<ALLOC_BUFFER_STATIC, ALLOC_PATTERN_FIRST_FIT>;
template class SequentialListAllocator<ALLOC_BUFFER_VMDYNAMIC, ALLOC_PATTERN_FIRST_FIT>;
template class SequentialListAllocator<ALLOC_BUFFER_STATIC_PREALLOC, ALLOC_PATTERN_BEST_FIT>;
template class SequentialListAllocator<ALLOC_BUFFER_STATIC, ALLOC_PATTERN_BEST_FIT>;
template class SequentialListAllocator<ALLOC_BUFFER_VMDYNAMIC, ALLOC_PATTERN_BEST_FIT>;