//
// Created by dsj on 2026/3/25.
//

#ifndef POOL_RADIXTREE_H
#define POOL_RADIXTREE_H
#include "Common.h"
#include "ObjectPool.h"
//三层基数树
template <int BITS>
class TCMalloc_PageMap3
{
private:
    static const int INTERIOR_BITS = (BITS + 2) / 3;       //第一、二层对应页号的比特位个数
    static const int INTERIOR_LENGTH = 1 << INTERIOR_BITS; //第一、二层存储元素的个数
    static const int LEAF_BITS = BITS - 2 * INTERIOR_BITS; //第三层对应页号的比特位个数
    static const int LEAF_LENGTH = 1 << LEAF_BITS;         //第三层存储元素的个数
    struct Node
    {
        Node* ptrs[INTERIOR_LENGTH];
    };
    struct Leaf
    {
        void* values[LEAF_LENGTH];
    };
    Node* NewNode()
    {
        // static ObjectPool<Node> nodePool;
        Node* result = (Node*)SystemAlloc((sizeof(Node) + (1 << PAGE_SHIFT) - 1) >> PAGE_SHIFT);
        if (result != NULL)
        {
            memset(result, 0, sizeof(*result));
        }
        return result;
    }
    Node* root_;
public:
    typedef uintptr_t Number;
    explicit TCMalloc_PageMap3()
    {
        root_ = NewNode();
    }
    void* get(Number k) const
    {
        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);         //第一层对应的下标
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1); //第二层对应的下标
        const Number i3 = k & (LEAF_LENGTH - 1);                    //第三层对应的下标
        //页号超出范围，或映射该页号的空间未开辟
        if ((k >> BITS) > 0 || root_->ptrs[i1] == NULL || root_->ptrs[i1]->ptrs[i2] == NULL)
        {
            return NULL;
        }
        return reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3]; //返回该页号对应span的指针
    }
    void set(Number k, void* v)
    {
        assert(k >> BITS == 0);
        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);         //第一层对应的下标
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1); //第二层对应的下标
        const Number i3 = k & (LEAF_LENGTH - 1);                    //第三层对应的下标
        Ensure(k, 1); //确保映射第k页页号的空间是开辟好了的
        reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3] = v; //建立该页号与对应span的映射
    }
    //确保映射[start,start+n-1]页号的空间是开辟好了的
    bool Ensure(Number start, size_t n)
    {
        for (Number key = start; key <= start + n - 1;)
        {
            const Number i1 = key >> (LEAF_BITS + INTERIOR_BITS);         //第一层对应的下标
            const Number i2 = (key >> LEAF_BITS) & (INTERIOR_LENGTH - 1); //第二层对应的下标
            if (i1 >= INTERIOR_LENGTH || i2 >= INTERIOR_LENGTH) //下标值超出范围
                return false;
            if (root_->ptrs[i1] == NULL) //第一层i1下标指向的空间未开辟
            {
                //开辟对应空间
                Node* n = NewNode();
                if (n == NULL) return false;
                root_->ptrs[i1] = n;
            }
            if (root_->ptrs[i1]->ptrs[i2] == NULL) //第二层i2下标指向的空间未开辟
            {
                //开辟对应空间
                // static ObjectPool<Leaf> leafPool;
                Leaf* leaf = (Leaf*)SystemAlloc((sizeof(Leaf) + (1 << PAGE_SHIFT) - 1) >> PAGE_SHIFT);
                if (leaf == NULL) return false;
                memset(leaf, 0, sizeof(*leaf));
                root_->ptrs[i1]->ptrs[i2] = reinterpret_cast<Node*>(leaf);
            }
            key = ((key >> LEAF_BITS) + 1) << LEAF_BITS; //继续后续检查
        }
        return true;
    }
    void PreallocateMoreMemory()
    {}
};

#endif //POOL_RADIXTREE_H
