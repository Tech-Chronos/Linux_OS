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
    // 出循环说明没有当前的桶没有span，可以解锁
    list._mutex.unlock();

    // 出来说明没找到span，要从page cache中获取
    PageCache* page_instance = PageCache::GetSingleton();
    // 访问 page 要加锁
    page_instance->_page_mutex.lock();
    Span* span = page_instance->NewSpan(SizeClass::NumMovePage(size));
    page_instance->_page_mutex.unlock();

    // 拿到了span，对span的操作，是线程私有的，不需要加锁
    assert(span);

    span->_obj_size = size;
    span->_use_count = 0;
    // 获取之后要进行切割，放到freelist中，根据页号算出起始地址
    char* begin = (char*)((span->_page_id) << PAGE_SHIFT);
    // 计算出总共的大小
    size_t bytes = ((span->_npage) << PAGE_SHIFT);
    char* end = begin + bytes;

    span->_free_lists = begin;
    begin += size;

    // 把 span 切成定长小块并串成单链表
    void* tail = span->_free_lists;
    while (begin < end)
    {
        NextObj(tail) = begin;
        tail = begin;
        begin += size;
    }
    NextObj(tail) = nullptr;

    // 将span插入list中，可能会干扰其他线程获取span，要加锁
    list._mutex.lock();
    // 要把span插入到list
    list.Push_Front(span);
    list._mutex.unlock();

    return span;
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

void CentralCache::ReleaseListToSpans(void* start, size_t size)
{

}

