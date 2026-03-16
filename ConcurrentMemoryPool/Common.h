//
// Created by dsj on 2026/3/14.
//

#ifndef CONCURRENTMEMORYPOOL_COMMON_H
#define CONCURRENTMEMORYPOOL_COMMON_H

#include <iostream>
#include <algorithm>
#include <vector>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <cassert>
using std::cout;
using std::endl;

#if defined(__LP64__) || defined(_WIN64)
    using PAGEID = unsigned long long;
#else
    using PAGEID = size_t;
#endif

static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREE_LISTS = 208;

inline static void*& NextObj(void* obj)
{
    return *(void**)obj;
}

class FreeList
{
public:
    FreeList()
        :_freelist(nullptr)
    {}

    void Push(void* obj)
    {
        assert(obj);
        NextObj(obj) = _freelist;
        _freelist = obj;
    }

    void* Pop()
    {
        assert(_freelist);
        void* obj = _freelist;
        _freelist = NextObj(obj);
        return obj;
    }

    bool Empty()
    {
        return _freelist == nullptr;
    }

    size_t& GetMaxSize()
    {
        return _max_size;
    }

private:
    void* _freelist;
    size_t _max_size = 1;
};

class SizeClass
{
private:
    // align_size 表示对齐数，size 表示你需要的空间是多大，返回值是我实际给你的空间大小
    static size_t _RoundUp(size_t size, size_t align_size)
    {
//        if (size % align_size == 0)
//        {
//            return size;
//        }
//        return ((size / align_size) + 1) * align_size;
        return (size + align_size - 1) & (~(align_size - 1));
    }

    static size_t _Index(size_t bytes, size_t align_shift)
    {
//        if (bytes % align_shift == 0)
//        {
//            return bytes / align_shift - 1;
//        }
//        else
//        {
//            return bytes / align_shift;
//        }
        return ((bytes + (1 << align_shift) - 1)) / (1 << align_shift) - 1;
    }
public:
    // 当用户要开辟Size空间，计算我要给多少空间
    static size_t RoundUp(size_t size)
    {
        // 必须小于 256 KB，不然要去Central Cache取
        assert(size < MAX_BYTES);

        // 整体的浪费空间的大小控制在 10% 左右
        if (size <= 128) //128 B, 8B 对齐
        {
            return _RoundUp(size, 8);
        }
        else if (size <= 1024) // 1 KB，16B 对齐
        {
            return _RoundUp(size, 16);
        }
        else if (size <= 8 * 1024) // 8 KB，128B 对齐
        {
            return _RoundUp(size, 128);
        }
        else if (size <= 64 * 1024) // 64 KB，1024B 对齐
        {
            return _RoundUp(size, 1024);
        }
        else if (size <= 256 * 1024) // 256 KB，8 * 1024B 对齐
        {
            return _RoundUp(size, 8 * 1024);
        }
        else
        {
            assert(false);
            return -1;
        }
    }

    // 计算下标
    static inline size_t Index(size_t bytes)
    {
        assert(bytes <= MAX_BYTES);

        size_t group[4] = {16 ,56, 56, 56};
        if (bytes <= 128)
        {
            return _Index(bytes, 3);
        }
        else if (bytes <= 1024)
        {
            return _Index(bytes - 128, 4) + group[0];
        }
        else if (bytes <= 8 * 1024)
        {
            return _Index(bytes - 1024, 7) + group[0] + group[1];
        }
        else if (bytes <= 64 * 1024)
        {
            return _Index(bytes - 8 * 1024, 10) + group[0] + group[1] + group[2];
        }
        else if (bytes <= 256 * 1024)
        {
            return _Index(bytes - 64 * 1024, 13) + group[0] + group[1] + group[2] + group[3];
        }
        else
        {
            assert(false);
            return -2;
        }
    }

    // 不能只从中心缓存拿一个内存块，也不能拿很少
    static size_t NumMoveSize(size_t size)
    {
        assert(size > 0);

        int nums = MAX_BYTES / size;

        if (nums < 2)
            return 2;

        if (nums > 512)
            return 512;

        return nums;
    }
};

// 大页
struct Span
{
    PAGEID _page_id; // 表示Span在虚拟地址空间的起始页编号，计算方法是 addr / 2^12
    size_t _npage; // 表示以page_id为起始值的一共有多少页

    void* _free_lists; // 每个span都有个自由链表表示链接的内存块

    Span* _prev;
    Span* _next;

    int _use_count; // 分出去的内存块的个数

    size_t _obj_size;
};


// 带头双向循环链表
class SpanList
{
public:
    SpanList()
    {
        _head = new Span;
        _head->_prev = _head;
        _head->_next = _head;
    }

    void Insert(Span* pos, Span* new_span)
    {
        assert(new_span);
        Span* prev = pos->_prev;

        prev->_next = new_span;
        new_span->_prev = prev;

        pos->_prev = new_span;
        new_span->_next = pos;
    }

    void Erase(Span* pos)
    {
        assert(pos);
        assert(_head != pos);

        Span* prev = pos->_prev;
        Span* next = pos->_next;

        prev->_next = next;
        next->_prev = prev;
    }

private:
    Span* _head;
public:
    std::mutex _mutex;
};

#endif //CONCURRENTMEMORYPOOL_COMMON_H
