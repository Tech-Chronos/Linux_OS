//
// Created by dsj on 2026/3/23.
//

#ifndef POOL_COMMON_H
#define POOL_COMMON_H
#include <iostream>
#include <unordered_map>
#include <vector>

#include <mutex>
#include <thread>

#include <algorithm>

#include <cassert>

using std::cout;
using std::endl;

typedef unsigned long long PAGEID;

static const int MAX_BYTES = 256 * 1024;
static const int NFREE_LIST = 209;
static const int NPAGE_LIST = 129;
static const int PAGE_SHIFT = 13;

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


static void*& NextObj(void*& obj)
{
    return *(void**)obj;
}

class SizeClass
{
private:
    static inline int _RoundUp(int size, int align_num)
    {
        return (size - 1 + align_num) & ~(align_num - 1);
    }

    static inline int _Index(int size, int align_shift)
    {
        return ((size - 1 + (1 << align_shift)) >> align_shift) - 1;
    }

public:
    // 计算对其数
    static int RoundUp(int size)
    {
        int align_size = 0;
        if (size <= (1 << 7)) // 3 次方 对齐
        {
            align_size = _RoundUp(size, 1 << 3);
        }
        else if (size <= (1 << 10)) // 4 次方 对齐
        {
            align_size = _RoundUp(size, 1 << 4);
        }
        else if (size <= (1 << 13)) // 7 次方 对齐
        {
            align_size = _RoundUp(size, 1 << 7);
        }
        else if (size <= (1 << 16)) // 10 次方 对齐
        {
            align_size = _RoundUp(size, 1 << 10);
        }
        else if (size <= (1 << 18)) // 13 次方 对齐
        {
            align_size = _RoundUp(size, 1 << 13);
        }
        return align_size;
    }

    // 计算在第几个哈希桶中（下标）
    static int Index(int size)
    {
        int index = 0;
        int group[4] = {16, 56, 56, 56};
        if (size <= (1 << 7)) // 3 次方 对齐
        {
            index = _Index(size, 3);
        }
        else if (size <= (1 << 10)) // 4 次方 对齐
        {
            index = _Index(size, 4) + group[0];
        }
        else if (size <= (1 << 13)) // 7 次方 对齐
        {
            index = _Index(size, 7) + group[0] + group[1];
        }
        else if (size <= (1 << 16)) // 10 次方 对齐
        {
            index = _Index(size, 10) + group[0] + group[1] + group[2];
        }
        else if (size <= (1 << 18)) // 13 次方 对齐
        {
            index = _Index(size, 13) + group[0] + group[1] + group[2] + group[3];
        }

        return index;
    }

    static inline int NumsFromCentralCache(int size)
    {
        assert(size < MAX_BYTES);
        int ret = MAX_BYTES / size;

        if (ret > 512)
            ret = 512;
        else if (ret < 2)
            ret = 2;

        return ret;
    }

    static inline int NumsFronPageCache(int size)
    {
        int nums = NumsFromCentralCache(size);

        int bytes = size * nums;

        // 按照页对齐计算bytes字节需要多少页
        int pages = (bytes - 1 + (1 << PAGE_SHIFT)) >> PAGE_SHIFT;

        return pages;
    }
};

// ThreadCache 中的自由链表
class FreeList
{
public:
    FreeList() = default;

    void push_front(void* obj)
    {
        NextObj(obj) = _free_list;
        _free_list = obj;
        ++_size;
    }

    void* pop_front()
    {
        void* ret = _free_list;
        _free_list = NextObj(_free_list);
        --_size;

        return ret;
    }

    void push_range_obj(void* start, void* end, int nums)
    {
        NextObj(end) = _free_list;
        _free_list = start;

        _size += nums;
    }

    void pop_range_obj(void*& start, void*& end, int nums)
    {
        // assert(nums <= _size);
        start = _free_list;
        end = start;

        int n = nums;

        // 要拿走 nums 个，走 nums - 1 步
        while (--n)
        {
            if (!end)
                int x = 0;
            end = NextObj(end);
        }
        _free_list = NextObj(end);

        NextObj(end) = nullptr;

        _size -= nums;
    }

    bool Empty()
    {
        return _free_list == nullptr;
    }

    int& MaxSize()
    {
        return _max_size;
    }

    int Size()
    {
        return _size;
    }
private:
    void* _free_list = nullptr;
    int _max_size = 1; // 相当于是一个阈值，最多向CentralCache中申请的内存块的大小
    int _size = 0; // 当前链表中的内存块个数
};

//  CentralCache 和 PageCache
struct Span
{
    Span()
        :_prev(nullptr)
        ,_next(nullptr)
        ,_free_list(nullptr)
        ,_id(0)
        ,_use_count(0)
        ,_n_pages(0)
        ,_is_used(false)
        ,_obj_size(0)
    {}

    Span* _prev;
    Span* _next;

    PAGEID _id; // 根据虚拟地址转化的页号
    size_t _n_pages; // 这个 span 占了多少页
    size_t _use_count; // 分配给 thread_cache 多少个内存块

    void* _free_list;
    bool _is_used;

    int _obj_size;
};

// 双向带头循环链表
class SpanList
{
public:
    SpanList()
        :_head(new Span)
    {
        _head->_prev = _head;
        _head->_next = _head;
    }

    Span* begin()
    {
        return _head->_next;
    }

    Span* end()
    {
        return _head;
    }

    void push_front(Span* new_span)
    {
        Span* next = _head->_next;

        new_span->_next = next;
        new_span->_prev = _head;

        next->_prev = new_span;
        _head->_next = new_span;
    }

    // 在page cache中，弹出一个给central cache用
    Span* pop_back()
    {
        Span* tail = _head->_prev;
        Span* tail_prev = tail->_prev;

        tail_prev->_next = _head;
        _head->_prev = tail_prev;

        tail->_next = nullptr;
        tail->_prev = nullptr;

        return tail;
    }

    void erase(Span* span)
    {
        assert(span);
        assert(span != _head);
        Span* prev = span->_prev;
        Span* next = span->_next;

        prev->_next = next;
        next->_prev = prev;

        span->_prev = nullptr;
        span->_next = nullptr;
    }

    bool empty()
    {
        return _head->_next == _head;
    }

private:
    Span* _head;
public:
    std::mutex _mtx;
};

#endif //POOL_COMMON_H
