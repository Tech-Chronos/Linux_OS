//
// Created by dsj on 2026/3/15.
//
#ifndef CONCURRENTMEMORYPOOL_CONCURRENT_H
#define CONCURRENTMEMORYPOOL_CONCURRENT_H

#include "ThreadCache.h"
#include "PageCache.h"

// TLS，线程中私有
static __thread ThreadCache* pTLSThreadCache = nullptr;

static void* ConcurrentMalloc(size_t size)
{
    if (size > MAX_BYTES)
    {
        // size 大于256KB 小于PAGE SIZE 的最大页，也就是 128页，每页4KB
        // 直接向PAGE CACHE申请
        size_t align_size = SizeClass::RoundUp(size);
        size_t k_page = align_size >> PAGE_SHIFT; // 计算出需要多少页

        PageCache::GetSingleton()->_page_mutex.lock();
        Span* new_span = PageCache::GetSingleton()->NewSpan(k_page);
        new_span->_obj_size = size;
        new_span->_use_count = 1;
        new_span->_is_used = true;
        new_span->_free_lists = nullptr;

        void* ptr = (void*)(new_span->_page_id << PAGE_SHIFT);

        PageCache::GetSingleton()->_page_mutex.unlock();
        return ptr;
    }
    else
    {
        if (!pTLSThreadCache)
        {
            pTLSThreadCache = new ThreadCache;
        }
        //cout << std::this_thread::get_id() <<": " << pTLSThreadCache << endl;
        return pTLSThreadCache->Allocate(size);
    }
}

static void ConcurrentDelete(void* ptr)
{
    Span* span = PageCache::GetSingleton()->PageToSpan(ptr);
    size_t size = span->_obj_size;
    if (size > MAX_BYTES)
    {
        PageCache::GetSingleton()->_page_mutex.lock();

        PageCache::GetSingleton()->ReleaseSpanToPageCache(span);

        PageCache::GetSingleton()->_page_mutex.unlock();
    }
    else
    {
        assert(pTLSThreadCache);
        pTLSThreadCache->DeAllocate(ptr, size);
    }
}

#endif //CONCURRENTMEMORYPOOL_CONCURRENT_H
