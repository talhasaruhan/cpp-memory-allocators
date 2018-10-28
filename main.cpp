#include "LinearAllocator.h"
#include "StackAllocator.h"
#include "PoolAllocator.h"
#include "SequentialListAllocator.h"
#include "RBTreeAllocator.h"
#include "SystemAllocator.h"
#include "AllocatorBenchmark.h"
#include <iostream>
#include <Windows.h>
#include "Util.h"

int main() {
    //void* p = malloc( 64* 1024*1024 );
    //SequentialListAllocator<ALLOC_BUFFER_VMDYNAMIC, ALLOC_PATTERN_FIRST_FIT> sqlAllocator(64 *1024* 1024);
    //SystemAllocator sysAllocator;
    //AllocatorBenchmark ab;

    //std::cout << "\n##########################################";
    //std::cout << "\n##########################################";
    //std::cout << "\n##########################################\n";
    //std::cout << "SEQUENTIAL LIST ALLOCATOR BENCHMARK\n";

    //ab.Benchmark(&sqlAllocator, AllocatorBenchmark::ALLOC_SEQ | AllocatorBenchmark::FREE_FIFO | 
    //    AllocatorBenchmark::FREE_LIFO | AllocatorBenchmark::FREE_RAND);

    //std::cout << "\n##########################################";
    //std::cout << "\n##########################################";
    //std::cout << "\n##########################################";
    //std::cout << "SYSTEM ALLOCATOR BENCHMARK\n";
    //ab.Benchmark(&sysAllocator, AllocatorBenchmark::ALLOC_SEQ | AllocatorBenchmark::ALLOC_SEQ |
    //    AllocatorBenchmark::FREE_FIFO | AllocatorBenchmark::FREE_LIFO | AllocatorBenchmark::FREE_RAND);

    //GPCL::DiscreteMultiRBTree<int> tree;

    //tree.Insert(30);
    //tree.Insert(20);
    //tree.Insert(40);
    //tree.Insert(50);
    //tree.Insert(35);

    //tree.Layout();

    //tree.Delete(40);

    //tree.Layout();

    RBTreeAllocator alloc(64*1024);
    alloc.Layout();
    void* ptr1 = alloc.Alloc(64, 8);
    alloc.Layout();
    alloc.Free(ptr1);
    alloc.Layout();

    return 0;
}
