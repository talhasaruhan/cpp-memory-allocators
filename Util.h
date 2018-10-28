#pragma once

#include <utility>
#include <cstdint>

// general purpose container library
namespace GPCL {
    
    // Toy pair
    template <typename T, typename Y>
    struct pair{
        T first;
        Y second;

        pair() {};
    
        pair(const pair<T, Y>& _pair) { 
            first = _pair.first; 
            second = _pair.second; 
        }
    
        pair(T _first, Y _second) : first(_first), second(_second) {};

        inline pair<T, Y> operator=(const pair<T, Y>& _pair) { 
            first = _pair.first; 
            second = _pair.second; 
            return *this; 
        }
    };

    template <typename T, typename Y>
    inline pair<T, Y> make_pair(T _first, Y _second) { return pair<T, Y>(_first, _second); }

    /******************************/
    /* A RB Tree that allows multiple inserts with the same key */
    /* Functions as a set by can be extended to a map just by adding a value variable to node *
    /* When used in the memory allocator context, the address of the node ptr itself is the value 
    */

    namespace RBTREE_HELPER {
        enum COLOR : uint8_t {
            BLACK,
            RED
        };

        template <class T>
        class Node {
        public:
            Node* parent;
            Node* left;
            Node* right;
            COLOR color;

            T key;

            Node* llPrev;
            Node* llNext;
            
            Node* sibling() {
                if (this->parent) {
                    if (this->parent->left == this)
                        return this->parent->right;
                    else
                        return this->parent->left;
                }
                return NULL;
            }
        };

    }

    template <class T>
    class DiscreteMultiRBTree {
    public:
        typedef RBTREE_HELPER::Node<T> Node;
        typedef RBTREE_HELPER::COLOR COLOR;
        
        Node* root = NULL;
        
        void Insert(T key) {
            _insert(root, key);
        }
        
        void Layout() {
            int i = 0, l;
            GPCL::pair<Node*, int> stack[64];
            stack[0] = GPCL::make_pair(root, 0);

            Node* cur;
            Node* temp;

            while (i >= 0) {
                cur = stack[i].first;
                l = stack[i--].second;
                if (!cur)
                    continue;
                for (int j = 0; j < l; ++j)
                    std::cout << " ";
                std::cout << cur->key << " | " << (void*)cur << " | ";
                //printf("%d | %p | ", cur->key, (void*)cur);
                
                temp = cur->llNext;
                while (temp) {
                    std::cout << cur->key << " | " << (void*)cur << " | ";
                    //printf("%d | %p | ", cur->key, (void*)cur);
                    temp = temp->llNext;
                }

                std::cout << "\n";

                stack[++i] = GPCL::make_pair(cur->left, l + 1);
                stack[++i] = GPCL::make_pair(cur->right, l+1);
            }

            std::cout << std::endl;
            // recursive
            //_layout(root, 0);
        }

        void Delete(T key) {
            Node* cur = root;
            Node* parent = NULL;

            while (cur) {
                parent = cur;
                if (key > cur->key) {
                    cur = cur->right;
                }
                else if (key < cur->key) {
                    cur = cur->left;
                }
                else {
                    // node exists and it's the head of LL
                    // if multiple nodes exist in the LL, just remove the head
                    Node* nextNode = cur->llNext;
                    if (nextNode) {
                        // NOTE that in any tree operation, only the head of the LL changes,
                        // thus we'll need to propogate changes to the next node
                        nextNode->llPrev = NULL;
                        nextNode->color = cur->color;
                        nextNode->left = cur->left;
                        nextNode->right = cur->right;
                        nextNode->parent = cur->parent;

                        // update the parent s.t. it holds a link to the new head
                        if (cur->parent) {
                            if (cur == cur->parent->left)
                                cur->parent->left = nextNode;
                            else
                                cur->parent->right = nextNode;
                        }

                        // update the children
                        if (cur->left)
                            cur->left->parent = nextNode;
                        if (cur->right)
                            cur->right->parent = nextNode;

                        return;
                    }

                    // if cur is the only node with the given key, do a tree deletion
                    _delete(cur);

                    return;
                }
            }

            // node with given key value doesn't exist!
            assert(false && "node with the given key doesn't exist!");
            return;
        }
 
    private:

        void _printNode(Node* node, int printMemAddr = false) {
            if (!printMemAddr)
                std::cout << node->key << "\n";
            else
                std::cout << node->key << " " << (uintptr_t)node << "\n";
        }

        void _layout(Node* node, int l, int printMemAddr = false) {
            if (!node)
                return;
            for (int j = 0; j < l; ++j)
                std::cout << " ";
            if (!printMemAddr)
                std::cout << node->key << "\n";
            else
                std::cout << node->key << " " << (uintptr_t)node << "\n";
            _layout(node->left, l+1);
            _layout(node->right, l+1);
        }

        static void _insert(Node*& root, T key) {
            Node* node = new Node;
            node->key = key;

            Node* cur = root;
            Node* parent = NULL;

            while (cur) {
                parent = cur;
                if (key > cur->key) {
                    cur = cur->right;
                }else if (key < cur->key) {
                    cur = cur->left;
                }else {
                    // node with a same key exists,
                    // insert node to the front of the LL
                    node->parent = cur->parent;
                    node->left = cur->left;
                    node->right = cur->right;
                    node->color = cur->color;
                    node->llPrev = NULL;
                    node->llNext = cur;
                    cur->llPrev = node;
                    
                    // parents always hold a link to the first node in LL
                    if (cur->parent) {
                        if (cur == cur->parent->left)
                            cur->parent->left = node;
                        else
                            cur->parent->right = node;
                    }

                    // so do the children
                    if (cur->left)
                        cur->left->parent = node;
                    if (cur->right)
                        cur->right->parent = node;
                        
                    return;
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
                return;
            }
            
            node->color = COLOR::RED;

            if (key > parent->key) {
                parent->right = node;
            }else {
                parent->left = node;
            }

            if(parent->parent)
                _fixup(root, node);

            return;
        }

        Node* _smallestInSubtree(Node* node) {
            while (node->left)
                node = node->left;
            return node;
        }

        Node* _BSTSubst(Node* node) {
            if (node->left && node->right)
                return _smallestInSubtree(node->right);
            if (!node->left && !node->right)
                return NULL;

            if (node->left)
                return node->left;
            else
                return node->right;
        }

        void _fixDoubleBlack(Node* node) {
            if (node == root)
                return;

            Node* parent = node->parent;
            Node* sibling = node->sibling();

            if (!sibling) {
                _fixDoubleBlack(parent);
            }
            else {
                bool siblingOnLeft = sibling->parent->left == sibling;

                if (sibling->color == COLOR::RED) {
                    parent->color = COLOR::RED;
                    sibling->color = COLOR::BLACK;
                    if (siblingOnLeft) {
                        _rotateRight(root, parent);
                    }
                    else {
                        _rotateLeft(root, parent);
                    }
                    _fixDoubleBlack(node);
                }
                else {
                    if ((sibling->left && sibling->left->color == COLOR::RED) ||
                        (sibling->right && sibling->right->color == COLOR::RED)) {

                        if (sibling->left && sibling->left->color == COLOR::RED) {
                            if (siblingOnLeft) {
                                sibling->left->color = sibling->color;
                                sibling->color = parent->color;
                                _rotateRight(root, parent);
                            }
                            else {
                                sibling->left->color = parent->color;
                                _rotateRight(root, sibling);
                                _rotateLeft(root, parent);
                            }
                        }
                        else {
                            if (siblingOnLeft) {
                                sibling->right->color = parent->color;
                                _rotateLeft(root, sibling);
                                _rotateRight(root, parent);
                            }
                            else {
                                sibling->right->color = sibling->color;
                                sibling->color = parent->color;
                                _rotateLeft(root, parent);
                            }
                        }
                        parent->color = COLOR::BLACK;
                    }
                    else {
                        sibling->color = COLOR::RED;
                        if (parent->color == COLOR::BLACK)
                            _fixDoubleBlack(parent);
                        else
                            parent->color = COLOR::BLACK;
                    }
                }
            }
        }

        void _delete(Node* node) {
            Node* subst = _BSTSubst(node);
            bool bothBlack = ((!subst || subst->color == COLOR::BLACK) && node->color == COLOR::BLACK);
            Node* parent = node->parent;

            if (!subst) {
                // subst NULL => node must be leaf
                if (node == root) {
                    root = NULL;
                }
                else {
                    if (bothBlack) {
                        _fixDoubleBlack(node);
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
                delete node;
                return;
            }

            if (!node->left || !node->right) {
                // node has 1 child
                if (node == root) {
                    /*node->key = subst->key;
                    node->left = NULL;
                    node->right = NULL;*/

                    root = subst;

                    delete node; // del subst
                }
                else {
                    // Detach v from tree and move u up 
                    if (parent->left == node)
                        parent->left = subst;
                    else
                        parent->right = subst;

                    delete node;

                    subst->parent = parent;

                    if (bothBlack)
                        _fixDoubleBlack(subst);
                    else
                        subst->color = COLOR::BLACK;
                }
                return;
            }

            // address consistency for memory models
            _swapNodes(node, subst);
            _delete(node);

            /* if address consistency is no issue, use this instead */
            /*T substKey = subst->key;
            subst->key = node->key;
            node->key = substKey;

            _delete(subst);*/
        }
        
        // swap the positions of the nodes in the graph while keeping their addresses consistent
        static void _swapNodes(Node* node,  Node* subst) {
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

        static void _fixup(Node *&root, Node *&pt)
        {
            Node *parent_pt = NULL;
            Node *grand_parent_pt = NULL;

            while ((pt != root) && (pt->color != COLOR::BLACK) &&
                (pt->parent->color == COLOR::RED))
            {

                parent_pt = pt->parent;
                grand_parent_pt = pt->parent->parent;

                /*  Case : A
                Parent of pt is left child of Grand-parent of pt */
                if (parent_pt == grand_parent_pt->left)
                {

                    Node *uncle_pt = grand_parent_pt->right;

                    /* Case : 1
                    The uncle of pt is also red
                    Only Recoloring required */
                    if (uncle_pt != NULL && uncle_pt->color == COLOR::RED)
                    {
                        grand_parent_pt->color = COLOR::RED;
                        parent_pt->color = COLOR::BLACK;
                        uncle_pt->color = COLOR::BLACK;
                        pt = grand_parent_pt;
                    }

                    else
                    {
                        /* Case : 2
                        pt is right child of its parent
                        Left-rotation required */
                        if (pt == parent_pt->right)
                        {
                            _rotateLeft(root, parent_pt);
                            pt = parent_pt;
                            parent_pt = pt->parent;
                        }

                        /* Case : 3
                        pt is left child of its parent
                        Right-rotation required */
                        _rotateRight(root, grand_parent_pt);

                        COLOR t = parent_pt->color;
                        parent_pt->color = grand_parent_pt->color;
                        grand_parent_pt->color = t;

                        pt = parent_pt;
                    }
                }

                /* Case : B
                Parent of pt is right child of Grand-parent of pt */
                else
                {
                    Node *uncle_pt = grand_parent_pt->left;

                    /*  Case : 1
                    The uncle of pt is also red
                    Only Recoloring required */
                    if ((uncle_pt != NULL) && (uncle_pt->color == COLOR::RED))
                    {
                        grand_parent_pt->color = COLOR::RED;
                        parent_pt->color = COLOR::BLACK;
                        uncle_pt->color = COLOR::BLACK;
                        pt = grand_parent_pt;
                    }
                    else
                    {
                        /* Case : 2
                        pt is left child of its parent
                        Right-rotation required */
                        if (pt == parent_pt->left)
                        {
                            _rotateRight(root, parent_pt);
                            pt = parent_pt;
                            parent_pt = pt->parent;
                        }

                        /* Case : 3
                        pt is right child of its parent
                        Left-rotation required */
                        _rotateLeft(root, grand_parent_pt);
                        
                        COLOR t = parent_pt->color;
                        parent_pt->color = grand_parent_pt->color;
                        grand_parent_pt->color = t;

                        pt = parent_pt;
                    }
                }
            }

            root->color = COLOR::BLACK;
        }

        static void _rotateLeft(Node *&root, Node *&pt)
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

        static void _rotateRight(Node *&root, Node *&pt)
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
    };

}