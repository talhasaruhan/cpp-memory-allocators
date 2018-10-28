#pragma once

#include "Allocator.h"
#include "Util.h"
#include "VMLinearAllocator.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <memory>

/*
* TODO
* Share functionality between classes using compile-time code generation
* Right now, it follows STATIC mode
* Since the mechanism is working, port the STATIC_PREALLOC and VMDYNAMIC modes
*/

class RBTreeAllocator : public Allocator
{
public:
    RBTreeAllocator(size_t sz);
    ~RBTreeAllocator();

    void* Alloc(size_t sz, size_t alignment) final;

    void Free(void*&) final;
    inline void Release() final;
    inline void Reset() final;
    inline void ZeroMem() final;

    void Layout() final;

protected:
    enum COLOR : uint8_t {
        BLACK,
        RED
    };

    class Node {
    public:
        Node* parent;
        Node* left;
        Node* right;
        COLOR color;

        uintptr_t sz;

        Node* llPrev;
        Node* llNext;

        Node* sibling();
    };

    struct FreeBlockHeader {
        Node rbtreeNode;
        size_t sz;
    };

    struct AllocatedBlockHeader {
        size_t sz;
        size_t padding;
    };

private:
    void build();

    static Node* _rbtreeInsert(Node*& root, Node* node, size_t key);
    static void _rbtreeFixup(Node *&root, Node *&pt);
    static void _rbtreeRotateLeft(Node *&root, Node *&pt);
    static void _rbtreeRotateRight(Node *&root, Node *&pt);
    Node* _smallestInSubtree(Node* node);
    Node* _BSTSubst(Node* node);
    void _rbtreeFixDoubleBlack(Node* node);
    void _rbtreeDelete(Node* node);
    void _rbtreeSwapNodes(Node* node, Node* subst);

    Node* _rbtreeFindKey(size_t key);
    void* _rbtreeStrictBestFit(Node* node, size_t key, size_t alignment, Node*& block, ptrdiff_t& padding);
    
    void _deleteNode(Node* node);

    inline void _fitPadding(ptrdiff_t& padding, size_t alignment);
    inline size_t _splitBlock(void* block, void* ptr, size_t sz);
    inline void* _fitToBlock(void* block, size_t sz, size_t alignment, ptrdiff_t& leftover, ptrdiff_t& padding);

    size_t m_size;

    Node* m_root = NULL;
    void* m_start;
    void* m_end;

    bool m_initialized;

    /* CONSTEXPRS */

    static constexpr size_t allocHeaderSize = sizeof(AllocatedBlockHeader);
    static constexpr size_t freeHeaderSize = sizeof(FreeBlockHeader);
};

