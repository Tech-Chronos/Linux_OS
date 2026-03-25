//
// Created by dsj on 2026/3/24.
//

#ifndef POOL_PAGECACHE_H
#define POOL_PAGECACHE_H
#include "Common.h"
#include "RadixTree.h"

class PageCache
{
public:
    PageCache() = default;
public:
    PageCache(const PageCache&) = delete;
    PageCache& operator=(const PageCache&) = delete;

    static PageCache* Singleton();

    Span* GetNewSpan(int page_num);

    // 通过地址返回对应的Span
    Span* AddrToSpan(void* addr);

    void ReleaseSpanToPageCache(Span* span);
private:
    SpanList _span_lists[NPAGE_LIST];
    //std::unordered_map<PAGEID, Span*> _id_span_map;
    TCMalloc_PageMap3<64 - PAGE_SHIFT> _radix_tree;
    static ObjectPool<PageCache> _pool;
    static ObjectPool<Span> pool;
public:
    std::mutex _mtx;
};


#endif //POOL_PAGECACHE_H
