# Custom Memory Allocators in C++
Multiple custom allocators written in C++ for my renderer, it's still incomplete and has somewhat inconsistent API, but this can be regarded as a personal experiment, and I'm more concerned with trying new things rather than having a shippable software package.

## Allocators and worst case complexities for alloc & dealloc (N: # of free blocks):
* **Linear**, O(1), O(1)
  + Also known as frame allocator, this class only allows user to allocate or deallocate the memory as a whole.
* **Pool**, O(1), O(1)
  + Can perform free operation but only allows allocating and deallocating certain sized blocks. It's absurdly simple but most of the time it works wonders. 
* **Stack**, O(1), O(1)
  + This allocator can deallocate the last allocated block.
* **Sequential Lists**, O(N), O(N)
  + Can allocate and deallocate blocks of any size. But it's a terrible general purpose allocator. 
  It can be used as a higher level memory manager, while managing the allocated blocks with seperate, more efficient allocators.
* **RBTree**, O(log(N)), O(log(N))
  + An allocator that constructs an Red Black Tree out of the unused blocks in the memory arena. 

## Some remarks:
* A basic benchmarking tool that measures the performance of allocation and free operations. Currently it accepts a union of these flags:
  + ALLOC_RAND, ALLOC_SEQ, FREE_LIFO, FREE_FIFO, FREE_RAND
* Can initialize with **STATIC**, **STATIC_PREALLOC**, **VMDYNAMIC** modes.
  + STATIC: Let the allocator commit a static pool memory.  
  + STATIC_PREALLLOC: Allow a preallocated block to be managed by the allocator.  
  + VMDYNAMIC: Use Win32 API to allocate a huge contiguous memory block, which then can be committed as needed.
* Try to give the user freedom to choose between first/best fit scenerios whenever possible.
* Tries to minimize 

## What I intent to work on next:
* Have a proper benchmarking tool that allocates & frees in a randomly fashion rather than having a long alloc or free strike one after other.
* Get RBTreeAllocator to coalesce adjacent free blocks.
* Implement a segregated lists algorithm.
* Homogenize the API across different allocators (template arguments etc.).

## Exposed API for the base class:
```
class Allocator {
public:
    ...
    // allocate
    virtual void* Alloc(size_t sz, size_t alignment) =0;   
    // deallocate
    virtual void Free(void*&) =0;
    // reset allocator state
    virtual void Reset() =0;
    // override memory region with zeroes
    virtual void ZeroMem() =0;
    // release (if exists) the buffer allocated from the OS at the initialiation of the allocator.
    virtual void Release() = 0;
    // print the memory layout for debugging purposes
    virtual void Layout() =0;
};

```
