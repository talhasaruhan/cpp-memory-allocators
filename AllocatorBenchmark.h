#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <algorithm>
#include "Allocator.h"

class AllocatorBenchmark {
public:
    enum FLAGS {
        ALLOC_SEQ = 1,
        ALLOC_RANDOM = 2,
        FREE_LIFO = 4,
        FREE_FIFO = 8,
        FREE_RAND = 16,
        ALLOC_FREE_RAND = 32
    };

    AllocatorBenchmark();
    ~AllocatorBenchmark();

    void Benchmark(Allocator*, int);

private:
    const static uint32_t N_TESTS = 10000;
    const static uint32_t alignment = 8;
    const static size_t* allocSize;

    // allocate batches of K byte blocks
    void allocSeq(Allocator*, void**);
    // allocate randomly sized blocks
    void allocRand(Allocator*, void**);
    void freeLIFO(Allocator*, void**);
    void freeFIFO(Allocator*, void**);
    void freeRand(Allocator*, void**);
};