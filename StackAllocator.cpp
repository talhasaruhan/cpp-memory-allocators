#include "StackAllocator.h"

StackAllocator::StackAllocator(size_t sz) : m_size(sz), m_free(sz), m_preAlloc(false), m_initialized(false) {
    m_start = (uint8_t*)malloc(m_size * sizeof(uint8_t));
    m_end = (void*) ((uintptr_t)m_start + m_size * sizeof(uint8_t));
    m_cur = m_start;
    m_initialized = true;
}

StackAllocator::StackAllocator(void* buffer, size_t sz) : m_size(sz), m_free(sz), m_preAlloc(true), m_initialized(true) {
    m_start = buffer;
    m_end = (void*)((uintptr_t)m_start + m_size * sizeof(uint8_t));
}

StackAllocator::~StackAllocator() {
    if (m_preAlloc || !m_initialized)
        return;
    free(m_start);
}

void StackAllocator::Layout() {
    std::cout << ((uintptr_t)m_end - (uintptr_t)m_start) << std::endl;
}

void* StackAllocator::Alloc(size_t sz, size_t alignment) {
    if (!m_initialized)
        return nullptr;

    assert((alignment & (alignment - 1)) == 0);

    uintptr_t ptr = ((uintptr_t)m_cur + alignment - 1) & ~(alignment - 1);

    size_t headerSize = sizeof(AllocHeader);
    ptrdiff_t padding = ptr - (uintptr_t)m_cur;

    // if can't fit header into the padding
    if (padding < headerSize) {
        if ((headerSize - padding) % alignment == 0) {
            padding = headerSize;
        }else {
            padding += alignment * (1 + (headerSize - padding)/alignment);
        }
    }

    ptr = (uintptr_t)m_cur + padding;
    uintptr_t blockEnd = ptr + sz;

    if (blockEnd > (uintptr_t)m_end) {
        assert(false && "Stack allocator full!");
        return nullptr;
    }

    void* headerPtr = (void*)((uintptr_t)ptr - headerSize);
    AllocHeader* header = new(headerPtr) AllocHeader;
    header->padding = padding;

    m_free -= m_size + padding;
    m_cur = (void*)blockEnd;

    //std::cout << "Allocated from " << (uintptr_t)ptr - (uintptr_t)m_start << " to " << (uintptr_t)ptr + sz - (uintptr_t)m_start
    //    << "  Header from " << (uintptr_t)ptr - (uintptr_t)m_start - sizeof(AllocHeader) << " to " <<
    //    (uintptr_t)ptr - (uintptr_t)m_start << std::endl;

    return (void*)ptr;
}

void StackAllocator::Free(void*& ptr) {
    if (!ptr)
        return;

    AllocHeader* header = (AllocHeader*) ((uintptr_t)ptr - sizeof(AllocHeader));
    m_cur = (void*) ((uintptr_t)ptr - header->padding);
    ptr = NULL;
}

void StackAllocator::Release() {
    if (m_preAlloc)
        return;
    free(m_start);
    m_initialized = false;
}

void StackAllocator::Reset() {
    m_cur = m_start;
}

void StackAllocator::ZeroMem() {
    memset(m_start, 0, m_size);
}
