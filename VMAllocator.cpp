#include "VMAllocator.h"
#include <assert.h>
#include <iostream>
#include <algorithm>

VMAllocator::VMAllocator(size_t sz) {
    SYSTEM_INFO sSysInfo;
    GetSystemInfo(&sSysInfo);
    m_pgSize = sSysInfo.dwPageSize;

    m_numPages = (uint32_t)ceil(sz / m_pgSize);
    m_size = m_numPages * m_pgSize;

    m_start = VirtualAlloc(NULL, m_size, MEM_RESERVE, PAGE_READWRITE);
    m_end = (void*)((uintptr_t)m_start + m_size);
    m_reserved = m_start;
}

VMAllocator ::~VMAllocator() {
}

// retrieve N contagious pages
void* VMAllocator::Alloc(uint32_t n) {
    assert(n>0);

    size_t allocSize = n * m_pgSize;

    // commit memory from the end of the reserved virtual address space
    // we don't need to worry about fragmentation in virtual address space
    // since for 64-bit systems it's practically unlimited

    if ((uintptr_t)m_reserved + allocSize > (uintptr_t)m_end)
        return NULL;

    void* p = VirtualAlloc(m_reserved, allocSize, MEM_COMMIT, PAGE_READWRITE);
    m_reserved = (void*)((uintptr_t)m_reserved + allocSize);

    AllocHeader* allocHeader = new(p) AllocHeader;
    allocHeader->n = n;

    return (void*)((uintptr_t)p + allocHeaderSize);
}

void VMAllocator::Free(void* ptr) {
    // from MSDN:
    // Reserved pages can be released only by freeing the entire block that was initially reserved by VirtualAlloc.
    // Thus, we're not releasing virtual memory, just decommitting it.

    void* p = (void*)((uintptr_t)ptr - allocHeaderSize);
    AllocHeader* allocHeader = (AllocHeader*)p;
    uint32_t blockSize = allocHeader->n*m_pgSize;

    // if the freed block is the last committed page, decrement the m_reserved ptr as well,
    // thus, we can guarantee that every new allocation starts from the END in suballocators

    if ((uintptr_t)p + blockSize == (uintptr_t)m_reserved)
        m_reserved = p;

    VirtualFree(p, blockSize, MEM_DECOMMIT);
}

void VMAllocator::Release() {
    VirtualFree(m_start, (uintptr_t)m_reserved-(uintptr_t)m_start, MEM_DECOMMIT);
    m_reserved = m_start;
}

size_t VMAllocator::PageSize() {
    return (size_t)m_pgSize;
}


