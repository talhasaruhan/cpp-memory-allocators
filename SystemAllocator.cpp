#include "SystemAllocator.h"

/* Constructors */

SystemAllocator::SystemAllocator(){
}

/* Destructor */

SystemAllocator::~SystemAllocator() {
}

void* SystemAllocator::Alloc(size_t sz, size_t alignment) {
    return _aligned_malloc(alignment, sz);
}

void SystemAllocator::Free(void*& p) {
    _aligned_free(p);
}

void SystemAllocator::Release() {
}

void SystemAllocator::Reset() {
}

void SystemAllocator::ZeroMem() {
}

void SystemAllocator::Layout() {
}
