//
// Created by dsj on 2026/3/16.
//
#include "CentralCache.h"

std::mutex CentralCache::_singlton_mutex;
CentralCache* CentralCache::_central_singlton = nullptr;

// 获取一个 span
Span* CentralCache::FetchOneSpan(SpanList& list, size_t size)
{
    Span* it = list.Begin();
    while (it != list.End())
    {
        // 判断freelist中是否有切好的内存块，主要是要freelist
        if (it->_free_lists != nullptr)
        {
            return it;
        }
        it = it->_next;
    }
    return nullptr;
}

// distribute_nums:分配多少个内存块，size:每块多大
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t distribute_nums, size_t size)
{
    // 计算出下标
    size_t index = SizeClass::Index(size);

    // 对桶加锁
    _span_lists[index]._mutex.lock();
    // 在对应的下标中获取一个 span
    Span* span = FetchOneSpan(_span_lists[index], size);
    if (span == nullptr)
    {
        _span_lists[index]._mutex.unlock();

        PageCache* page_instance = PageCache::GetSingleton();
        page_instance->_page_mutex.lock();
        span = page_instance->NewSpan(SizeClass::NumMovePage(size));
        page_instance->_page_mutex.unlock();

        assert(span);
        span->_obj_size = size;
        span->_use_count = 0;
        span->_is_used = true;

        char* begin = (char*)((span->_page_id) << PAGE_SHIFT);
        size_t bytes = ((span->_npage) << PAGE_SHIFT);
        char* end_bound = begin + bytes;
        assert(bytes >= size);
        assert(begin < end_bound);

        span->_free_lists = begin;
        begin += size;

        void* tail = span->_free_lists;
        while (begin < end_bound)
        {
            NextObj(tail) = begin;
            tail = begin;
            begin += size;
        }
        NextObj(tail) = nullptr;

        _span_lists[index]._mutex.lock();
        _span_lists[index].Push_Front(span);
    }
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
    span->_use_count += actual_num;

    NextObj(end) = nullptr;
    _span_lists[index]._mutex.unlock();

    return actual_num;
}

// 归还的可能不止一个span
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
    size_t index = SizeClass::Index(size);
    // 加桶锁
    _span_lists[index]._mutex.lock();
    while (start)
    {
        void* next = NextObj(start);
        Span* span = PageCache::GetSingleton()->PageToSpan(start);

        NextObj(start) = span->_free_lists;
        span->_free_lists = start;
        --span->_use_count;

        // 对于某个span已经没有使用的小块内存了
        if (span->_use_count == 0)
        {
            _span_lists[index].Erase(span);
            span->_free_lists = nullptr;
            span->_prev = nullptr;
            span->_next = nullptr;

            // 对这个span修改好，可以解锁
            _span_lists[index]._mutex.unlock();

            PageCache::GetSingleton()->_page_mutex.lock();

            PageCache::GetSingleton()->ReleaseSpanToPageCache(span);
            PageCache::GetSingleton()->_page_mutex.unlock();

            _span_lists[index]._mutex.lock();
        }

        start = next;
    }

    _span_lists[index]._mutex.unlock();
}
