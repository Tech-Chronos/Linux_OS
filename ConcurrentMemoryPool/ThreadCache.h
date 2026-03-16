//
// Created by dsj on 2026/3/14.
//

#ifndef CONCURRENTMEMORYPOOL_THREADCACHE_H
#define CONCURRENTMEMORYPOOL_THREADCACHE_H
#include "Common.h"


class ThreadCache
{
public:
    void* Allocate(size_t size);

    void DeAllocate(void* ptr, size_t size);

    void* FetchFromCentralCache(size_t bytes);

private:
    FreeList _freelists[NFREE_LISTS];
};

#endif //CONCURRENTMEMORYPOOL_THREADCACHE_H
