#include "VMLinearAllocator.h"
#include <assert.h>
#include <iostream>
#include <algorithm>

VMLinearAllocator::VMLinearAllocator(size_t sz) {
    SYSTEM_INFO sSysInfo;
    GetSystemInfo(&sSysInfo);
    m_pgSize = sSysInfo.dwPageSize;

    m_numPages = (uint32_t)ceil(sz / m_pgSize);
    m_size = m_numPages * m_pgSize;

    m_start = VirtualAlloc(NULL, m_size, MEM_RESERVE, PAGE_READWRITE);
    m_end = (void*)((uintptr_t)m_start + m_size);
    m_reserved = m_start;
}

VMLinearAllocator ::~VMLinearAllocator() {
}

// retrieve N contagious pages
void* VMLinearAllocator::Alloc(uint32_t n) {
    assert(n>0);

    size_t allocSize = n * m_pgSize;

    // commit memory from the end of the reserved virtual address space
    // we don't need to worry about fragmentation in virtual address space
    // since for 64-bit systems it's practically unlimited

    if ((uintptr_t)m_reserved + allocSize > (uintptr_t)m_end)
        return NULL;

    void* p = VirtualAlloc(m_reserved, allocSize, MEM_COMMIT, PAGE_READWRITE);
    m_reserved = (void*)((uintptr_t)m_reserved + allocSize);

    return p;
}

void VMLinearAllocator::Free(void* ptr) {
    assert(false, "Can't free vmlinear allocator\n");
}

void VMLinearAllocator::Release() {
    VirtualFree(m_start, (uintptr_t)m_reserved-(uintptr_t)m_start, MEM_DECOMMIT);
    m_reserved = m_start;
}

size_t VMLinearAllocator::PageSize() {
    return (size_t)m_pgSize;
}


