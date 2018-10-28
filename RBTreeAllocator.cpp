#include "RBTreeAllocator.h"

RBTreeAllocator::Node* RBTreeAllocator::Node::sibling() {
    if (this->parent) {
        if (this->parent->left == this)
            return this->parent->right;
        else
            return this->parent->left;
    }
    return NULL;
}

RBTreeAllocator::RBTreeAllocator(size_t sz): m_size(sz), m_initialized(false), m_root(NULL), m_start(NULL), m_end(NULL)
{
    // only STATIC mode for now,
    // once the data structure is working, it's quite easy to port PREALLOC & VMDYNAMIC modes
    m_start = (uint8_t*)malloc(m_size * sizeof(uint8_t));
    m_end = (void*)((uintptr_t)m_start + m_size * sizeof(uint8_t));
    m_initialized = true;
    build();
}

RBTreeAllocator::~RBTreeAllocator()
{
}

void RBTreeAllocator::build() {
    m_root = new(m_start) Node{NULL, NULL, NULL, COLOR::BLACK, m_size, NULL, NULL};
}

size_t RBTreeAllocator::_splitBlock(void* block, void* ptr, size_t sz) {
    Node* blockHeader = (Node*)block;

    ptrdiff_t remainingSpace = (uintptr_t)block + blockHeader->sz - (uintptr_t)ptr - sz;

    _deleteNode((Node*)block);

    if (remainingSpace >= freeHeaderSize) {
        // create a new free block
        void* newBlock = (void*)((uintptr_t)ptr + sz);

        Node* newNode = new(newBlock) Node;
        size_t freeBlockSize = blockHeader->sz - ((uintptr_t)ptr - (uintptr_t)block + sz);
        newNode->sz = freeBlockSize;

        _rbtreeInsert(m_root, newNode, freeBlockSize);

        return sz;
    }
    else {
        // cant create a new free block because remaining space is not large enough for header
        return (uintptr_t)blockHeader->sz - (uintptr_t)ptr + (uintptr_t)block;
    }

    return 0;
}

void RBTreeAllocator::_fitPadding(ptrdiff_t& padding, size_t alignment) {
    // if can't fit header into the padding
    if (padding < allocHeaderSize) {
        if ((allocHeaderSize - padding) % alignment == 0) {
            padding = allocHeaderSize;
        }
        else {
            padding += alignment * (1 + (allocHeaderSize - padding) / alignment);
        }
    }
}

void* RBTreeAllocator::_fitToBlock(
    void* block, size_t sz, size_t alignment, ptrdiff_t& leftover, ptrdiff_t& padding)
{
    void* ptr = (void*)(((uintptr_t)block + alignment - 1) & ~(alignment - 1));
    padding = (uintptr_t)ptr - (uintptr_t)block;
    _fitPadding(padding, alignment);
    ptr = (void*)((uintptr_t)block + padding);
    leftover = (ptrdiff_t)((Node*)block)->sz - (padding + sz);
    return ptr;
}

void * RBTreeAllocator::Alloc(size_t sz, size_t alignment)
{
    assert((alignment & (alignment - 1)) == 0);

    Node* block = NULL;
    ptrdiff_t padding;

    void* ptr = _rbtreeStrictBestFit(m_root, sz, alignment, block, padding);

    AllocatedBlockHeader* allocHeader = new((void*)((uintptr_t)ptr - allocHeaderSize)) AllocatedBlockHeader;
    allocHeader->sz = _splitBlock(block, ptr, sz);
    allocHeader->padding = padding;

    return ptr;
}

/*
* Currently, we don't coalesce the free blocks
*/

void RBTreeAllocator::Free(void*& ptr)
{
    if (!ptr)
        return;

    AllocatedBlockHeader* allocHeader = (AllocatedBlockHeader*)((uintptr_t)ptr - allocHeaderSize);
    size_t allocSize = allocHeader->sz;
    size_t allocPadding = allocHeader->padding;

    void* freeHeaderPtr = (void*)((uintptr_t)ptr - allocPadding);

    Node* newNode = new(freeHeaderPtr) Node;
    size_t freeBlockSize = allocSize + allocPadding;
    newNode->sz = freeBlockSize;

    _rbtreeInsert(m_root, newNode, freeBlockSize);
}

inline void RBTreeAllocator::Release()
{
}

inline void RBTreeAllocator::Reset()
{
}

inline void RBTreeAllocator::ZeroMem()
{
}

void RBTreeAllocator::Layout()
{
    int i = 0, l;
    GPCL::pair<Node*, int> stack[64];
    stack[0] = GPCL::make_pair(m_root, 0);

    Node* cur;
    Node* temp;

    while (i >= 0) {
        cur = stack[i].first;
        l = stack[i--].second;
        if (!cur)
            continue;
        for (int j = 0; j < l; ++j)
            std::cout << " ";
        std::cout << cur->sz << " | " << (void*)cur << " | ";
        //printf("%d | %p | ", cur->key, (void*)cur);

        temp = cur->llNext;
        while (temp) {
            std::cout << cur->sz << " | " << (void*)cur << " | ";
            //printf("%d | %p | ", cur->key, (void*)cur);
            temp = temp->llNext;
        }

        std::cout << "\n";

        stack[++i] = GPCL::make_pair(cur->left, l + 1);
        stack[++i] = GPCL::make_pair(cur->right, l + 1);
    }

    std::cout << std::endl;
}

RBTreeAllocator::Node* RBTreeAllocator::_rbtreeInsert(Node*& root, Node* node, size_t key) {
    node->sz = key;

    Node* cur = root;
    Node* parent = NULL;

    while (cur) {
        parent = cur;
        if (key > cur->sz) {
            cur = cur->right;
        }
        else if (key < cur->sz) {
            cur = cur->left;
        }
        else {
            node->parent = cur->parent;
            node->left = cur->left;
            node->right = cur->right;
            node->color = cur->color;
            node->llPrev = NULL;
            node->llNext = cur;
            cur->llPrev = node;

            if (cur->parent) {
                if (cur == cur->parent->left)
                    cur->parent->left = node;
                else
                    cur->parent->right = node;
            }

            if (cur->left)
                cur->left->parent = node;
            if (cur->right)
                cur->right->parent = node;

            return node;
        }
    }

    node->parent = parent;
    node->left = NULL;
    node->right = NULL;
    node->llPrev = NULL;
    node->llNext = NULL;

    if (!parent) {
        root = node;
        node->color = COLOR::BLACK;
        return node;
    }

    node->color = COLOR::RED;

    if (key > parent->sz) {
        parent->right = node;
    }
    else {
        parent->left = node;
    }

    if (parent->parent)
        _rbtreeFixup(root, node);

    return node;
}

void RBTreeAllocator::_rbtreeFixup(Node *&root, Node *&pt)
{
    Node *parent_pt = NULL;
    Node *grand_parent_pt = NULL;

    while ((pt != root) && (pt->color != COLOR::BLACK) &&
        (pt->parent->color == COLOR::RED))
    {
        parent_pt = pt->parent;
        grand_parent_pt = pt->parent->parent;

        if (parent_pt == grand_parent_pt->left)
        {

            Node *uncle_pt = grand_parent_pt->right;

            if (uncle_pt != NULL && uncle_pt->color == COLOR::RED)
            {
                grand_parent_pt->color = COLOR::RED;
                parent_pt->color = COLOR::BLACK;
                uncle_pt->color = COLOR::BLACK;
                pt = grand_parent_pt;
            }

            else
            {
                if (pt == parent_pt->right)
                {
                    _rbtreeRotateLeft(root, parent_pt);
                    pt = parent_pt;
                    parent_pt = pt->parent;
                }

                _rbtreeRotateRight(root, grand_parent_pt);

                COLOR t = parent_pt->color;
                parent_pt->color = grand_parent_pt->color;
                grand_parent_pt->color = t;

                pt = parent_pt;
            }
        }

        else
        {
            Node *uncle_pt = grand_parent_pt->left;

            if ((uncle_pt != NULL) && (uncle_pt->color == COLOR::RED))
            {
                grand_parent_pt->color = COLOR::RED;
                parent_pt->color = COLOR::BLACK;
                uncle_pt->color = COLOR::BLACK;
                pt = grand_parent_pt;
            }
            else
            {
                if (pt == parent_pt->left)
                {
                    _rbtreeRotateRight(root, parent_pt);
                    pt = parent_pt;
                    parent_pt = pt->parent;
                }

                _rbtreeRotateLeft(root, grand_parent_pt);

                COLOR t = parent_pt->color;
                parent_pt->color = grand_parent_pt->color;
                grand_parent_pt->color = t;

                pt = parent_pt;
            }
        }
    }

    root->color = COLOR::BLACK;
}

void RBTreeAllocator::_rbtreeRotateLeft(Node *&root, Node *&pt)
{
    Node *pt_right = pt->right;

    pt->right = pt_right->left;

    if (pt->right != NULL)
        pt->right->parent = pt;

    pt_right->parent = pt->parent;

    if (pt->parent == NULL)
        root = pt_right;

    else if (pt == pt->parent->left)
        pt->parent->left = pt_right;

    else
        pt->parent->right = pt_right;

    pt_right->left = pt;
    pt->parent = pt_right;
}

void RBTreeAllocator::_rbtreeRotateRight(Node *&root, Node *&pt)
{
    Node *pt_left = pt->left;

    pt->left = pt_left->right;

    if (pt->left != NULL)
        pt->left->parent = pt;

    pt_left->parent = pt->parent;

    if (pt->parent == NULL)
        root = pt_left;

    else if (pt == pt->parent->left)
        pt->parent->left = pt_left;

    else
        pt->parent->right = pt_left;

    pt_left->right = pt;
    pt->parent = pt_left;
}

void RBTreeAllocator::_deleteNode(Node* node)
{
    // node is the head of LL, as all links from adjacent nodes are to the top of the LL
    // if multiple nodes exist in the LL, just remove the head
    Node* nextNode = node->llNext;
    if (nextNode) {
        // NOTE that in any tree operation, only the head of the LL changes,
        // thus we'll need to propogate changes to the next node
        nextNode->llPrev = NULL;
        nextNode->color = node->color;
        nextNode->left = node->left;
        nextNode->right = node->right;
        nextNode->parent = node->parent;

        // update the parent s.t. it holds a link to the new head
        if (node->parent) {
            if (node == node->parent->left)
                node->parent->left = nextNode;
            else
                node->parent->right = nextNode;
        }

        // update the children
        if (node->left)
            node->left->parent = nextNode;
        if (node->right)
            node->right->parent = nextNode;

        return;
    }

    // if cur is the only node with the given key, do a tree deletion
    _rbtreeDelete(node);

    return;
}

RBTreeAllocator::Node* RBTreeAllocator::_smallestInSubtree(Node* node) {
    while (node->left)
        node = node->left;
    return node;
}

RBTreeAllocator::Node* RBTreeAllocator::_BSTSubst(Node* node) {
    if (node->left && node->right)
        return _smallestInSubtree(node->right);
    if (!node->left && !node->right)
        return NULL;

    if (node->left)
        return node->left;
    else
        return node->right;
}

void RBTreeAllocator::_rbtreeFixDoubleBlack(Node* node) {
    if (node == m_root)
        return;

    Node* parent = node->parent;
    Node* sibling = node->sibling();

    if (!sibling) {
        _rbtreeFixDoubleBlack(parent);
    }
    else {
        bool siblingOnLeft = sibling->parent->left == sibling;

        if (sibling->color == COLOR::RED) {
            parent->color = COLOR::RED;
            sibling->color = COLOR::BLACK;
            if (siblingOnLeft) {
                _rbtreeRotateRight(m_root, parent);
            }
            else {
                _rbtreeRotateLeft(m_root, parent);
            }
            _rbtreeFixDoubleBlack(node);
        }
        else {
            if ((sibling->left && sibling->left->color == COLOR::RED) ||
                (sibling->right && sibling->right->color == COLOR::RED)) {

                if (!sibling->left && sibling->left->color == COLOR::RED) {
                    if (siblingOnLeft) {
                        sibling->left->color = sibling->color;
                        sibling->color = parent->color;
                        _rbtreeRotateRight(m_root, parent);
                    }
                    else {
                        sibling->left->color = parent->color;
                        _rbtreeRotateRight(m_root, sibling);
                        _rbtreeRotateLeft(m_root, parent);
                    }
                }
                else {
                    if (siblingOnLeft) {
                        sibling->right->color = parent->color;
                        _rbtreeRotateLeft(m_root, sibling);
                        _rbtreeRotateRight(m_root, parent);
                    }
                    else {
                        sibling->right->color = sibling->color;
                        sibling->color = parent->color;
                        _rbtreeRotateLeft(m_root, parent);
                    }
                }
                parent->color = COLOR::BLACK;
            }
            else {
                sibling->color = COLOR::RED;
                if (parent->color == COLOR::BLACK)
                    _rbtreeFixDoubleBlack(parent);
                else
                    parent->color = COLOR::BLACK;
            }
        }
    }
}

void RBTreeAllocator::_rbtreeDelete(Node* node) {
    Node* subst = _BSTSubst(node);
    bool bothBlack = ((!subst || subst->color == COLOR::BLACK) && node->color == COLOR::BLACK);
    Node* parent = node->parent;

    if (!subst) {
        // subst NULL => node must be leaf
        if (node == m_root) {
            m_root = NULL;
        }
        else {
            if (bothBlack) {
                _rbtreeFixDoubleBlack(node);
            }
            else {
                Node* sibling = node->sibling();
                if (sibling)
                    sibling->color = COLOR::RED;
            }

            if (parent->left == node)
                parent->left = NULL;
            else
                parent->right = NULL;
        }
        return;
    }

    if (!node->left || !node->right) {
        // node has 1 child
        if (node == m_root) {
            m_root = subst;
        }
        else {
            // Detach v from tree and move u up 
            if (parent->left == node)
                parent->left = subst;
            else
                parent->right = subst;

            subst->parent = parent;

            if (bothBlack)
                _rbtreeFixDoubleBlack(subst);
            else
                subst->color = COLOR::BLACK;
        }
        return;
    }

    // address consistency for memory models
    _rbtreeSwapNodes(node, subst);
    _rbtreeDelete(node);
}

// swap the positions of the nodes in the graph while keeping their addresses consistent
void RBTreeAllocator::_rbtreeSwapNodes(Node* node, Node* subst) {
    Node* parent = node->parent;

    Node temp;
    memcpy(&temp, node, sizeof(Node));

    if (parent->left == node)
        parent->left = subst;
    else
        parent->right = subst;

    if (subst->left)
        subst->left->parent = node;
    if (subst->right)
        subst->right->parent = node;

    node->parent = subst;
    node->left = subst->left;
    node->right = subst->right;


    subst->parent = temp.parent;
    if (temp.left == subst) {
        subst->left = node;
        subst->right = temp.right;
        temp.right->parent = subst;
    }
    else {
        subst->right = node;
        subst->left = temp.left;
        temp.left->parent = subst;
    }
}

RBTreeAllocator::Node* RBTreeAllocator::_rbtreeFindKey(size_t key) {
    Node* cur = m_root;
    Node* parent = NULL;

    while (cur) {
        parent = cur;
        if (key > cur->sz) {
            cur = cur->right;
        }
        else if (key < cur->sz) {
            cur = cur->left;
        }
        else {
            return cur;
        }
    }

    return NULL;
}

/*
* NOTE: THIS METHOD ASSUMES ARBITRARY ALIGNMENT
* Consequently, it can (however VERY unlikely) fall back to O(N) 
* if all free blocks 
*     have greater size than the allocation size, 
*     but can't allow required alignment and padding.
* This problem happens regardless of the data structure used, we need to define the problem better.
*
* Fortunetely, in a real world application, we know the maximum possible alignment that can be requested.
* Unless you want to waste memory for no reason, the maximum alignment can be set to 8 bytes.
* So, we can GUARANTEE O(log(N)) operation, with minimal to no amount of wasted space
*/

void* RBTreeAllocator::_rbtreeStrictBestFit(Node* node, size_t sz, size_t alignment, Node*& block, ptrdiff_t& padding) {
    printf("Current node at %p > %d\n", node, node->sz);

    if (!node || (!node->left && !node->right && node->sz < sz))
        return NULL;

    void* ptr;
    ptrdiff_t leftover;

    if (node->sz >= sz && (!node->left || node->left->sz < sz)) {
        // node has key gt sz, and left child has key lt sz, eliminate the left branch
        ptr = _fitToBlock(node, sz, alignment, leftover, padding);
        if (leftover > 0) {
            // node can accomodate for the padding and the header
            block = node;
            return ptr;
        }
        else {
            // if not, search right branch
            return _rbtreeStrictBestFit(node->right, sz, alignment, block, padding);
        }
    }

    if (node->sz <= sz) {
        return _rbtreeStrictBestFit(node->right, sz, alignment, block, padding);
    }
    else {
        // both node and the left child have keys greater than sz
        ptr = _rbtreeStrictBestFit(node->left, sz, alignment, block, padding);
        if (!ptr) {
            // despite having a key greater than sz, left child can't account for the padding & header
            ptr = _fitToBlock(node, sz, alignment, leftover, padding);
            if (leftover > 0) {
                // we fall back to this node again,
                block = node;
                return ptr;
            }
            else {
                // the worst case, we continue searching the right branch
                return _rbtreeStrictBestFit(node->right, sz, alignment, block, padding);
            }
        }
        else {
            return ptr;
        }
    }

}