//
// Created by dsj on 2026/3/25.
//

#ifndef POOL_CONCURRENT_H
#define POOL_CONCURRENT_H
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

__thread static ThreadCache* pTLSThreadCache = nullptr;

static void* Alloc(int size)
{
    if (pTLSThreadCache == nullptr)
    {
        //pTLSThreadCache = new ThreadCache;
        static std::mutex _mtx;
        static ObjectPool<ThreadCache> pool;
        _mtx.lock();
        pTLSThreadCache = pool.New(sizeof(ThreadCache));
        _mtx.unlock();
    }
    return pTLSThreadCache->Allocate(size);
}

static void Free(void* ptr)
{
    Span* span = PageCache::Singleton()->AddrToSpan(ptr);
    int size = SizeClass::RoundUp(span->_obj_size);

    assert(pTLSThreadCache);
    pTLSThreadCache->Deallocate(ptr, size);
}

#endif //POOL_CONCURRENT_H
