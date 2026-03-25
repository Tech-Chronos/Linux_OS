//
// Created by dsj on 2026/3/25.
//

#ifndef POOL_OBJECTPOOL_H
#define POOL_OBJECTPOOL_H
#include "Common.h"

template <class T>
class ObjectPool
{
public:
    T* New(size_t size = sizeof(T))
    {
        T* obj = nullptr;
        if (_freelist)
        {
            obj =  (T*)_freelist;
            _freelist = NextObj(_freelist); // 拿到前4or8字节的空间
        }
        else
        {
            size_t obj_size = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);

            if (_remain_size < obj_size)
            {
                _remain_size = 1 << 13;
                _memory = (char*)SystemAlloc(_remain_size >> 13);
                if (!_memory)
                    throw std::bad_alloc();
            }

            obj = (T*)_memory;
            _remain_size -= sizeof(T);
            _memory += sizeof(T);
        }

        new(obj) T;
        return obj;
    }

    void Delete(T* ptr)
    {
        assert(ptr);

        ptr->~T();

        *(void**)ptr = _freelist;
        _freelist = ptr;
    }

private:
    void* _freelist = nullptr; // 归还回来的内存放到这里
    char* _memory = nullptr; // 指向整体的内存块
    int _remain_size = 0; // 剩余空间的大小
};

#endif //POOL_OBJECTPOOL_H
