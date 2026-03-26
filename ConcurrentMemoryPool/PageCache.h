//
// Created by dsj on 2026/3/17.
//

#ifndef CONCURRENTMEMORYPOOL_PAGECACHE_H
#define CONCURRENTMEMORYPOOL_PAGECACHE_H
#include "Common.h"
#include "FixedMemoryPool.h"
#include "PageMap.h"

class PageCache
{
private:
    PageCache() = default;

public:
    PageCache(const PageCache&) = delete;
    PageCache& operator=(const PageCache&) = delete;

    static PageCache* GetSingleton()
    {
        static PageCache instance;
        return &instance;
    }

    Span* NewSpan(size_t npage);

    Span* PageToSpan(void*);

    void ReleaseSpanToPageCache(Span* span);

private:
    SpanList _span_lists[NPAGE_LISTS]; // 一个页对应一个桶
    ObjectPool<Span> _obj_pool;
    TCMalloc_PageMap3<(sizeof(PAGEID) * 8) - PAGE_SHIFT> _id_span_map;
public:
    std::mutex _page_mutex;
};

#endif //CONCURRENTMEMORYPOOL_PAGECACHE_H
