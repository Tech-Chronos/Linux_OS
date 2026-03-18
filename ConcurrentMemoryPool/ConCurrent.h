//
// Created by dsj on 2026/3/15.
//

#ifndef CONCURRENTMEMORYPOOL_CONCURRENT_H
#define CONCURRENTMEMORYPOOL_CONCURRENT_H

#include "ThreadCache.h"

// TLS，线程中私有
static __thread ThreadCache* pTLSThreadCache = nullptr;

static void* ConcurrentMalloc(size_t size)
{
    if (!pTLSThreadCache)
    {
        pTLSThreadCache = new ThreadCache;
    }
    //cout << std::this_thread::get_id() <<": " << pTLSThreadCache << endl;
    return pTLSThreadCache->Allocate(size);
}

static void ConcurrentDelete(void* ptr, size_t size)
{
    assert(pTLSThreadCache);
    pTLSThreadCache->DeAllocate(ptr, size);
}

#endif //CONCURRENTMEMORYPOOL_CONCURRENT_H
