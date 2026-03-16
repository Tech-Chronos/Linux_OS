//
// Created by dsj on 2026/3/16.
//
#include "CentralCache.h"

std::mutex CentralCache::_singlton_mutex;
CentralCache* CentralCache::_central_singlton = nullptr;

Span* CentralCache::FetchOneSpan(size_t index)
{

    return nullptr;
}

// distribute_nums:分配多少个内存块，size:每块多大
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t distribute_nums, size_t size)
{
    // 计算出下标
    size_t index = SizeClass::Index(size);

    // 在对应的下标中获取一个 span
    Span* span = FetchOneSpan(index);
    assert(span);
    assert(span->_free_lists);

    // 获取span的起始的地址
    start = span->_free_lists;
    end = start;

    size_t n = distribute_nums;
    size_t actual_num = 1;
    while (--n && NextObj(end) != nullptr)
    {
        end = NextObj(end);
        ++actual_num;
    }
    span->_free_lists = NextObj(end);

    NextObj(end) = nullptr;

    return actual_num;
}

