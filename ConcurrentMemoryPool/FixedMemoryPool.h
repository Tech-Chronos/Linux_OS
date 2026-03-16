//
// Created by dsj on 2026/3/14.
//

#ifndef CONCURRENTMEMORYPOOL_FIXEDMEMORYPOOL_H
#define CONCURRENTMEMORYPOOL_FIXEDMEMORYPOOL_H
#include <iostream>
#include <unistd.h>

using std::cout;
using std::endl;

template <class T>
class ObjectPool
{
public:
    ObjectPool()
        :_memory(nullptr)
         ,_free_list(nullptr)
         ,_remain_size(0)
    {}

    T* New()
    {
        T* cur = nullptr;
        // 先在自由链表中是否存在着T*的内存块，如果存在就直接给
        if (_free_list)
        {
            void* next = *((void**)_free_list);
            cur = (T*)_free_list;
            _free_list = next;
        }
        else
        {
            // 如果剩余的空间大小不够了那就重新分配
            if (_remain_size < sizeof(T))
            {
                _remain_size = 4*1024;
                char* tmp = (char*)malloc(_remain_size);
                if (!tmp)
                {
                    throw std::bad_alloc();
                }
                _memory = tmp;
            }
            cur = (T*)_memory;
            // 如果对象类型小于指针类型，就要分配指针类型的大小给上层，防止在delete的时候freelist不够存放一个指针
            int obj_size = sizeof(T) > sizeof(T*) ? sizeof(T) : sizeof(T*);
            _memory += obj_size;
            _remain_size -= obj_size;
        }
        new (cur) T;
        return cur;
    }

    void Delete(T* ptr)
    {
        ptr->~T();
        // 把这个内存块放入 freelist 中
        *(void**)ptr = _free_list;
        _free_list = ptr;
    }
private:
    char* _memory;
    void* _free_list;
    int _remain_size;
};


struct TreeNode
{
    int _val;
    TreeNode* _left;
    TreeNode* _right;
    TreeNode()
        : _val(0)
        , _left(nullptr)
        , _right(nullptr)
    {}
};

void TestObjectPool()
{
    // 申请释放的轮次
    const size_t Rounds = 10;
    // 每轮申请释放多少次
    const size_t N = 100000;
    size_t begin1 = clock();
    std::vector<TreeNode*> v1;
    v1.reserve(N);
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v1.push_back(new TreeNode);
        }
        for (int i = 0; i < N; ++i)
        {
            delete v1[i];
        }
        v1.clear();
    }

    size_t end1 = clock();
    ObjectPool<TreeNode> TNPool;
    size_t begin2 = clock();
    std::vector<TreeNode*> v2;
    v2.reserve(N);
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v2.push_back(TNPool.New());
        }
        for (int i = 0; i < 100000; ++i)
        {
            TNPool.Delete(v2[i]);
        }
        v2.clear();
    }
    size_t end2 = clock();
    cout <<"new cost time:" <<end1 - begin1 << endl;
    cout <<"object pool cost time:" <<end2 - begin2 << endl;
}

#endif //CONCURRENTMEMORYPOOL_FIXEDMEMORYPOOL_H
