//
// Created by dsj on 2026/3/23.
//

#ifndef POOL_CENTRALCACHE_H
#define POOL_CENTRALCACHE_H
#include "Common.h"
#include "ObjectPool.h"

class CentralCache
{
public:
    CentralCache(const CentralCache&) = delete;
    CentralCache& operator=(const CentralCache&) = delete;

    static CentralCache* Singleton();
    // 获取内存
    int FetchRangeObj(void*& start, void*& end, int size, int obj_num);

    Span* GetOneSpan(int size);

    Span* FetchFromPageCache(int size);

    // 释放内存
    void ReleaseToSpan(void*& start, void*& end, int size, int return_size);

public:
    CentralCache() = default;

private:
    SpanList _span_lists[NFREE_LIST];
    static ObjectPool<CentralCache> _pool;
};


#endif //POOL_CENTRALCACHE_H
