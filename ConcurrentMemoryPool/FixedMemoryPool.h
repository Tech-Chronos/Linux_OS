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
        size_t obj_size = sizeof(T) > sizeof(T*) ? sizeof(T) : sizeof(T*);
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
            if (_remain_size < obj_size)
            {
                size_t block_size = std::max((size_t)4 * 1024, obj_size);
                char* tmp = (char*)malloc(block_size);
                if (!tmp)
                {
                    throw std::bad_alloc();
                }
                _memory = tmp;
                _remain_size = block_size;
            }
            cur = (T*)_memory;
            // 如果对象类型小于指针类型，就要分配指针类型的大小给上层，防止在delete的时候freelist不够存放一个指针
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
    size_t _remain_size;
};

#endif //CONCURRENTMEMORYPOOL_FIXEDMEMORYPOOL_H
