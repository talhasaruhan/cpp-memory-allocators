#include "AllocatorBenchmark.h"
#include <random>

const size_t* AllocatorBenchmark::allocSize;

AllocatorBenchmark::AllocatorBenchmark() {
    allocSize = new size_t[8]{ 8, 16, 32, 64, 128, 256, 512, 1024 };
}

AllocatorBenchmark::~AllocatorBenchmark() {}

inline AllocatorBenchmark::FLAGS operator|(AllocatorBenchmark::FLAGS a, AllocatorBenchmark::FLAGS b)
{
    return (AllocatorBenchmark::FLAGS)((int)a | (int)b);
}

void AllocatorBenchmark::allocSeq(Allocator* allocator, void** ptr) {
    __int64 prevTime = 0, QPCfreq = 0, curTime = 0, deltaTicks = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&QPCfreq);
    double QPCperiod = 1.0f / QPCfreq, deltaTime = 0, totalTime = 0;

    std::cout << "\n/********************************/\nALLOC SEQ\n";
    for (int i = 0; i < 8; ++i) {
        deltaTicks = 0;

        for (int j = 0; j < N_TESTS; ++j) {
            QueryPerformanceCounter((LARGE_INTEGER*)&prevTime);
            void* p = allocator->Alloc(allocSize[i], alignment);
            QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
            deltaTicks += curTime - prevTime;

            ptr[i*N_TESTS + j] = p;
        }

        deltaTime = (deltaTicks) * (QPCperiod * 1000);
        totalTime += deltaTime;
        std::cout << N_TESTS << " Sequential Allocations w/ allocation size " << allocSize[i] << " took " << deltaTime << " ms\n";
    }

    std::cout << N_TESTS*8 << " Sequential Allocations took " << totalTime << " ms\n";
    std::cout << "ALLOC SEQ\n/********************************/\n\n";
}

void AllocatorBenchmark::allocRand(Allocator* allocator, void** ptr) {
    __int64 prevTime = 0, QPCfreq = 0, curTime = 0, totalTime = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&QPCfreq);
    double QPCperiod = 1.0f / QPCfreq, deltaTime = 0;

    std::cout << "\n/********************************/\nALLOC RAND\n";
    int tests[N_TESTS * 8];

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < N_TESTS; ++j) {   
            tests[i*N_TESTS + j] = allocSize[i];
        }
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(tests, tests+N_TESTS*8, g);

    for (int j = 0; j < N_TESTS*8; ++j) {
        QueryPerformanceCounter((LARGE_INTEGER*)&prevTime);
        void* p = allocator->Alloc(tests[j], alignment);
        QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
        
        totalTime += curTime - prevTime;

        ptr[j] = p;
    }

    deltaTime = (totalTime) * (QPCperiod * 1000);
    std::cout << N_TESTS*8 << " Random Allocations took " << deltaTime << " ms\n";

    std::cout << "ALLOC RAND\n/********************************/\n\n";
}

void AllocatorBenchmark::freeLIFO(Allocator* allocator, void** ptr) {
    __int64 prevTime = 0, QPCfreq = 0, curTime = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&QPCfreq);
    double QPCperiod = 1.0f / QPCfreq, deltaTime = 0, totalTime = 0;

    std::cout << "\n/********************************/\nFREE LIFO\n";
    for (int i = 7; i >= 0; --i) {
        QueryPerformanceCounter((LARGE_INTEGER*)&prevTime);
        for (int j = N_TESTS - 1; j >= 0; --j) {
            allocator->Free(ptr[i*N_TESTS + j]);
        }
        QueryPerformanceCounter((LARGE_INTEGER*)&curTime);

        deltaTime = ((curTime - prevTime) * QPCperiod);
        totalTime += deltaTime;
        std::cout << N_TESTS << " LIFO Sequential Free w/ allocation size " << allocSize[i] << " took " << deltaTime * 1000 << " ms\n";
        prevTime = curTime;
    }
    std::cout << N_TESTS * 8 << " LIFO free took " << totalTime << " ms\n";
    std::cout << "FREE LIFO\n/********************************/\n\n";
}

void AllocatorBenchmark::freeFIFO(Allocator* allocator, void** ptr) {
    __int64 prevTime = 0, QPCfreq = 0, curTime = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&QPCfreq);
    double QPCperiod = 1.0f / QPCfreq, deltaTime = 0, totalTime = 0;

    std::cout << "\n/********************************/\nFREE FIFO\n";
    for (int i = 0; i < 8; ++i) {
        QueryPerformanceCounter((LARGE_INTEGER*)&prevTime);
        for (int j = 0; j < N_TESTS; ++j) {
            allocator->Free(ptr[i*N_TESTS + j]);
        }
        QueryPerformanceCounter((LARGE_INTEGER*)&curTime);

        deltaTime = ((curTime - prevTime) * QPCperiod);
        totalTime += deltaTime;
        std::cout << N_TESTS << " FIFO Sequential Free w/ allocation size " << allocSize[i] << " took " << deltaTime * 1000 << " ms\n";
        prevTime = curTime;
    }
    std::cout << N_TESTS * 8 << " FIFO free took " << totalTime << " ms\n";
    std::cout << "FREE FIFO\n/********************************/\n\n";
}

void AllocatorBenchmark::freeRand(Allocator* allocator, void** ptr) {
    __int64 prevTime = 0, QPCfreq = 0, curTime = 0, totalTime = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&QPCfreq);
    double QPCperiod = 1.0f / QPCfreq, deltaTime = 0;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ptr, ptr + N_TESTS * 8, g);

    std::cout << "\n/********************************/\nRAND FREE\n";

    QueryPerformanceCounter((LARGE_INTEGER*)&prevTime);
    for (int j = 0; j < N_TESTS*8; ++j) {
        allocator->Free(ptr[j]);
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&curTime);

    deltaTime = ((curTime - prevTime) * QPCperiod);
    std::cout << N_TESTS*8 << " RAND Free took " << deltaTime * 1000 << " ms\n";

    std::cout << "RAND FREE\n/********************************/\n\n";
}

void AllocatorBenchmark::Benchmark(Allocator* allocator, int flags) {
    __int64 prevTime = 0, QPCfreq = 0, curTime = 0, totalTime = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&QPCfreq);
    double QPCperiod = 1.0f / QPCfreq, deltaTime = 0;

    if (!(flags & (ALLOC_SEQ | ALLOC_RANDOM)))
        return;

    // Keep track of allocations
    void* ptr[N_TESTS * 8];

    if ((bool)(flags & ALLOC_FREE_RAND)) {
        std::cout << "ALLOCATE AND FREE IN A RANDOM FASHION\n";
        // TODO allocFreeRand(allocator, ptr);
    }

    if((bool)(flags & FREE_LIFO) & (bool)(flags & FREE_FIFO) & (bool)(flags & FREE_RAND)){
        std::cout << "LIFO AND FIFO AND RAND\n";
        // LIFO AND FIFO

        if (bool(flags & ALLOC_SEQ) & bool(flags & ALLOC_RANDOM)) {
            std::cout << "SEQ AND RAND\n";
            allocSeq(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }else if (flags & ALLOC_SEQ) {
            std::cout << "SEQ BUT NOT RAND\n";
            allocSeq(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_RANDOM) {
            std::cout << "RAND BUT NOT SEQ\n";
            allocRand(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }

    }else if (bool(flags & FREE_LIFO) & bool(flags & FREE_FIFO)) {
        std::cout << "LIFO AND FIFO BUT NOT RAND\n";
        // LIFO AND FIFO BUT NOT RAND
        if (bool(flags & ALLOC_SEQ) & bool(flags & ALLOC_RANDOM)) {
            std::cout << "SEQ AND RAND\n";
            allocSeq(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_SEQ) {
            std::cout << "SEQ BUT NOT RAND\n";
            allocSeq(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_RANDOM) {
            std::cout << "RAND BUT NOT SEQ\n";
            allocRand(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

        }
    }else if (bool(flags & FREE_LIFO) & bool(flags & FREE_RAND)) {
        std::cout << "LIFO AND RAND BUT NOT FIFO\n";
        // LIFO AND RAND BUT NOT FIFO
        if (bool(flags & ALLOC_SEQ) & bool(flags & ALLOC_RANDOM)) {
            std::cout << "SEQ AND RAND\n";
            allocSeq(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_SEQ) {
            std::cout << "SEQ BUT NOT RAND\n";
            allocSeq(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_RANDOM) {
            std::cout << "RAND BUT NOT SEQ\n";
            allocRand(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
    }else if (bool(flags & FREE_RAND) & bool(flags & FREE_FIFO)) {
        std::cout << "RAND AND FIFO BUT NOT LIFO\n";
        // RAND AND FIFO BUT NOT LIFO
        if (bool(flags & ALLOC_SEQ) & bool(flags & ALLOC_RANDOM)) {
            std::cout << "SEQ AND RAND\n";
            allocSeq(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_SEQ) {
            std::cout << "SEQ BUT NOT RAND\n";
            allocSeq(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocSeq(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_RANDOM) {
            std::cout << "RAND BUT NOT SEQ\n";
            allocRand(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
    }else if (flags & FREE_RAND) {
        std::cout << "RAND BUT NOT FIFO OR LIFO\n";
        // RAND BUT NOT FIFO OR LIFO
        if (bool(flags & ALLOC_SEQ) & bool(flags & ALLOC_RANDOM)) {
            std::cout << "SEQ AND RAND\n";
            allocSeq(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_SEQ) {
            std::cout << "SEQ BUT NOT RAND\n";
            allocSeq(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_RANDOM) {
            std::cout << "RAND BUT NOT SEQ\n";
            allocRand(allocator, ptr);
            freeRand(allocator, ptr);
            allocator->Layout();

        }
    }
    else if (flags & FREE_LIFO) {
        std::cout << "LIFO BUT NOT FIFO OR RAND\n";
        // LIFO BUT NOT FIFO OR RAND

        if (bool(flags & ALLOC_SEQ) & bool(flags & ALLOC_RANDOM)) {
            std::cout << "SEQ AND RAND\n";
            allocSeq(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

        }else if (flags & ALLOC_SEQ) {
            std::cout << "SEQ BUT NOT RAND\n";
            allocSeq(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

        }else if (flags & ALLOC_RANDOM) {
            std::cout << "RAND BUT NOT SEQ\n";
            allocRand(allocator, ptr);
            freeLIFO(allocator, ptr);
            allocator->Layout();

        } 

    }else if (flags & FREE_FIFO) {
        std::cout << "FIFO BUT NOT LIFO OR RAND\n";
        // FIFO but NOT LIFO

        if (bool(flags & ALLOC_SEQ) & bool(flags & ALLOC_RANDOM)) {
            std::cout << "SEQ AND RAND\n";
            allocSeq(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

            allocRand(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

        }else if (flags & ALLOC_SEQ) {
            std::cout << "SEQ BUT NOT RAND\n";
            allocSeq(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

        }
        else if (flags & ALLOC_RANDOM) {
            std::cout << "RAND BUT NOT SEQ\n";
            allocRand(allocator, ptr);
            freeFIFO(allocator, ptr);
            allocator->Layout();

        }

    }else{
        // NO FREEING BENCHMARK
        
        if (bool(flags & ALLOC_SEQ) & bool(flags & ALLOC_RANDOM)) {
            std::cout << "SEQ AND RAND\n";
            allocSeq(allocator, ptr);
            allocator->Reset();
            allocator->Layout();

            allocRand(allocator, ptr);
            allocator->Reset();
            allocator->Layout();

        }
        else if (flags & ALLOC_SEQ) {
            std::cout << "SEQ BUT NOT RAND\n";
            allocSeq(allocator, ptr);
            allocator->Reset();
            allocator->Layout();

        }
        else if (flags & ALLOC_RANDOM) {
            std::cout << "RAND BUT NOT SEQ\n";
            allocRand(allocator, ptr);
            allocator->Reset();
            allocator->Layout();

        }
    }

}