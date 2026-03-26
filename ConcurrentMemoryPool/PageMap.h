//
// Created by dsj on 2026/3/22.
//

#ifndef CONCURRENTMEMORYPOOL_PAGEMAP_H
#define CONCURRENTMEMORYPOOL_PAGEMAP_H

#include "Common.h"
#include "FixedMemoryPool.h"

template <int BITS>
class TCMalloc_PageMap3
{
private:
    static const int INTERIOR_BITS = (BITS + 2) / 3;
    static const int INTERIOR_LENGTH = 1 << INTERIOR_BITS;
    static const int LEAF_BITS = BITS - 2 * INTERIOR_BITS;
    static const int LEAF_LENGTH = 1 << LEAF_BITS;

    struct Node
    {
        void* ptrs[INTERIOR_LENGTH];
    };

    struct Leaf
    {
        void* values[LEAF_LENGTH];
    };

    using Number = uintptr_t;

    Node* NewNode()
    {
        Node* node = _node_pool.New();
        memset(node, 0, sizeof(Node));
        return node;
    }

    Leaf* NewLeaf()
    {
        Leaf* leaf = _leaf_pool.New();
        memset(leaf, 0, sizeof(Leaf));
        return leaf;
    }

    Leaf* EnsureLeaf(Number k)
    {
        assert((k >> BITS) == 0);

        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1);

        if (_root->ptrs[i1] == nullptr)
        {
            _root->ptrs[i1] = NewNode();
        }

        Node* second = reinterpret_cast<Node*>(_root->ptrs[i1]);
        if (second->ptrs[i2] == nullptr)
        {
            second->ptrs[i2] = NewLeaf();
        }

        return reinterpret_cast<Leaf*>(second->ptrs[i2]);
    }

public:
    TCMalloc_PageMap3()
        : _root(NewNode())
    {}

    void* get(Number k) const
    {
        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1);
        const Number i3 = k & (LEAF_LENGTH - 1);

        if ((k >> BITS) != 0 || _root->ptrs[i1] == nullptr)
        {
            return nullptr;
        }

        Node* second = reinterpret_cast<Node*>(_root->ptrs[i1]);
        if (second->ptrs[i2] == nullptr)
        {
            return nullptr;
        }

        Leaf* leaf = reinterpret_cast<Leaf*>(second->ptrs[i2]);
        return leaf->values[i3];
    }

    void set(Number k, void* v)
    {
        const Number i3 = k & (LEAF_LENGTH - 1);
        Leaf* leaf = EnsureLeaf(k);
        leaf->values[i3] = v;
    }

    void erase(Number k)
    {
        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1);
        const Number i3 = k & (LEAF_LENGTH - 1);

        if ((k >> BITS) != 0 || _root->ptrs[i1] == nullptr)
        {
            return;
        }

        Node* second = reinterpret_cast<Node*>(_root->ptrs[i1]);
        if (second->ptrs[i2] == nullptr)
        {
            return;
        }

        Leaf* leaf = reinterpret_cast<Leaf*>(second->ptrs[i2]);
        leaf->values[i3] = nullptr;
    }

private:
    Node* _root;
    ObjectPool<Node> _node_pool;
    ObjectPool<Leaf> _leaf_pool;
};

#endif //CONCURRENTMEMORYPOOL_PAGEMAP_H
