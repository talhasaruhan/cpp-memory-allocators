#include "PoolAllocator.h"
#include <iostream>

PoolAllocator::PoolAllocator(size_t sz, size_t pgSz) :
    m_size(sz), m_pgSize(pgSz),
    m_preAlloc(false), m_initialized(false)
{
    assert(m_size % m_pgSize == 0);

    m_start = (uint8_t*)malloc(m_size * sizeof(uint8_t));
    m_end = (void*)((uintptr_t)m_start + m_size * sizeof(uint8_t));

    buildLinkedList();

    m_initialized = true;
}

void PoolAllocator::Layout() {
    for (uintptr_t ptr = (uintptr_t)m_llStart; ptr < (uintptr_t)m_end; ) {
        void* fPgPtr = ((FreePageHeader*)ptr)->ptr;
        std::cout << "Free Page: [" << ptr - (uintptr_t)m_start << " "
            << ptr - (uintptr_t)m_start + m_pgSize << "] Next: ";
        if (!fPgPtr) {
            std::cout << "NULL" << "\n";
            break;
        }
        std::cout << (uintptr_t)fPgPtr - (uintptr_t)m_start << " || ";
        ptr = (uintptr_t)((FreePageHeader*)ptr)->ptr;
    }
    std::cout << std::endl;
}

PoolAllocator::PoolAllocator(void* buffer, size_t sz, size_t pgSz) :
    m_size(sz), m_pgSize(pgSz),
    m_preAlloc(true), m_initialized(true)
{
    assert(m_size % m_pgSize == 0);

    m_start = buffer;
    m_end = (void*)((uintptr_t)m_start + m_size * sizeof(uint8_t));

    buildLinkedList();
}

PoolAllocator::~PoolAllocator() {
    if (m_preAlloc || !m_initialized)
        return;
    free(m_start);
}

void PoolAllocator::buildLinkedList() {
    FreePageHeader header;

    for (uintptr_t ptr = (uintptr_t)m_start; ptr <= (uintptr_t)m_end - 2*m_pgSize; ) {
        header.ptr = (void*)(ptr + m_pgSize);
        memcpy((void*)ptr, &header, sizeof(FreePageHeader));
        ptr += m_pgSize;
    }

    header.ptr = NULL;

    memcpy((void*)((uintptr_t)m_end - m_pgSize), &header, sizeof(FreePageHeader));

    m_llStart = m_start;
}

void* PoolAllocator::Alloc(size_t sz, size_t alignment) {
    if (!m_initialized || !m_llStart)
        return nullptr;

    assert((alignment & (alignment - 1)) == 0);
    
    // If you need to MALLOC_N multiple pages,
    // Use SequentialListAllocator where every allocation is an integer multiple of some page size.
    // This allocator is designed for low overhead and simplicity.
    assert(sz <= m_pgSize); 

    void* ptr = std::align(alignment, sz, m_llStart, m_pgSize);

    if (!ptr) {
        assert(false && "Linear allocator full!");
        return nullptr;
    }

    m_llStart = ((FreePageHeader*)m_llStart)->ptr;

    return ptr;
}

void PoolAllocator::Free(void*& ptr) {
    if (!ptr)
        return;

    // shift the ptr back to the page boundary
    if (((uintptr_t)ptr - (uintptr_t)m_start) % m_pgSize > 0) {
        ptr = (void*) ((((uintptr_t)ptr - (uintptr_t)m_start) / m_pgSize) * m_pgSize + (uintptr_t)m_start);
    }

    FreePageHeader* header = new(ptr) FreePageHeader;
    header->ptr = m_llStart;
    m_llStart = ptr;
    ptr = NULL;
}

void PoolAllocator::Release() {
    if (m_preAlloc)
        return;
    free(m_start);
    m_initialized = false;
}

void PoolAllocator::Reset() {
    buildLinkedList();
}

void PoolAllocator::ZeroMem() {
    memset(m_start, 0, m_size);
}
