#pragma once
#include <cstdint>
#include <Windows.h>

#define DEFAULT_VM_PAGE_SIZE 4096

class VMLinearAllocator {
public:
    VMLinearAllocator(size_t sz);
    ~VMLinearAllocator();

    void* Alloc(uint32_t n);
    void Free(void* ptr);
    void Release();

    size_t PageSize();

private:
    DWORD m_pgSize;
    uint32_t m_numPages;

    void* m_start;
    void* m_reserved;
    void* m_end;
    size_t m_size;
};