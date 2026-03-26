//
// Created by dsj on 2026/3/14.
//

#ifndef CONCURRENTMEMORYPOOL_COMMON_H
#define CONCURRENTMEMORYPOOL_COMMON_H

#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <cassert>
#include <cstddef>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

inline static void* SystemAlloc(size_t kpage)
{
    if (kpage == 0) return nullptr;

    size_t bytes = kpage << 13;

#ifdef _WIN32
    void* ptr = VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      if (ptr == nullptr) throw std::bad_alloc();
      return ptr;
#else
    int flags = MAP_PRIVATE;
#ifdef __APPLE__
    flags |= MAP_ANON;        // macOS
#else
    flags |= MAP_ANONYMOUS;   // Linux
#endif
    void* ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (ptr == MAP_FAILED) throw std::bad_alloc();
    return ptr;
#endif
}

inline static void SystemFree(void* ptr, size_t kpage)
{
    if (ptr == nullptr || kpage == 0) return;

    size_t bytes = kpage << 13;

#ifdef _WIN32
    BOOL ret = VirtualFree(ptr, 0, MEM_RELEASE);
      if (ret == 0) throw std::runtime_error("VirtualFree failed");
#else
    int ret = munmap(ptr, bytes);
    if (ret != 0) throw std::runtime_error("munmap failed");
#endif
}

using std::cout;
using std::endl;

#if defined(__LP64__) || defined(_WIN64)
using PAGEID = unsigned long long;
#else
using PAGEID = size_t;
#endif


static const size_t MAX_BYTES = 512 * 1024;
static const size_t NFREE_LISTS = 208;
static const size_t NPAGE_LISTS = 129;
static const size_t PAGE_SHIFT = 13;

inline static void*& NextObj(void* obj)
{
    return *(void**)obj;
}

class FreeList
{
public:
    FreeList()
        :_freelist(nullptr)
        ,_max_size(1)
        ,_size(0)
    {}

    void Push(void* obj)
    {
        assert(obj);
        NextObj(obj) = _freelist;
        _freelist = obj;
        ++_size;
    }

    void* Pop()
    {
        assert(_freelist);
        void* obj = _freelist;
        _freelist = NextObj(obj);
        --_size;
        return obj;
    }

    void Push_Range(void*& start, void*& end, size_t n)
    {
        assert(start && end);
        NextObj(end) = _freelist;

        _freelist = start;

        _size += n;
    }

    void Pop_Range(void*& start, void*& end, size_t n)
    {
        start = _freelist;
        end = start;

        for (int i = 0; i < n - 1; ++i)
        {
            end = NextObj(end);
        }
        _size -= n;
        _freelist = NextObj(end);

        NextObj(end) = nullptr;
    }

    bool Empty()
    {
        return _freelist == nullptr;
    }

    size_t& GetMaxSize()
    {
        return _max_size;
    }

    size_t Size()
    {
        return _size;
    }

private:
    void* _freelist;
    size_t _max_size;
    size_t _size;
};

class SizeClass
{
public:
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
            return _RoundUp(size, 1 << PAGE_SHIFT);
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

    static size_t NumMovePage(size_t size)
    {
        assert(size > 0);
        size_t nums = NumMoveSize(size); // 计算出需要在这个桶里分配多少内存块的上限
        size_t bytes = nums * size; // 计算出总共的大小

        size_t npage = (bytes + ((size_t)1 << PAGE_SHIFT) - 1) >> PAGE_SHIFT; // 向上取整到页数

        if (npage == 0)
            npage = 1;

        return npage;
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

    bool _is_used = false;
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

    Span* Begin()
    {
        return _head->_next;
    }

    Span* End()
    {
        return _head;
    }

    bool Empty()
    {
        return _head->_next == _head;
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

    Span* Erase(Span* pos)
    {
        assert(pos);
        assert(_head != pos);

        Span* prev = pos->_prev;
        Span* next = pos->_next;

        prev->_next = next;
        next->_prev = prev;

        return pos;
    }

    void Push_Front(Span* new_span)
    {
        Insert(Begin(), new_span);
    }

    Span* Pop_Front()
    {
        return Erase(Begin());
    }

    void Push_Back(Span* new_span)
    {
        Insert(End(), new_span);
    }

private:
    Span* _head;
public:
    std::mutex _mutex;
};

#endif //CONCURRENTMEMORYPOOL_COMMON_H
