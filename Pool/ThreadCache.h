//
// Created by dsj on 2026/3/23.
//

#ifndef POOL_THREADCACHE_H
#define POOL_THREADCACHE_H

#include "Common.h"

class ThreadCache
{
public:
    void* Allocate(int size);

    void* FetchFromCentralCache(int size);

    void Deallocate(void* ptr, int size);

    void ListTooLong(FreeList& list, int size);

private:
    FreeList _free_lists[NFREE_LIST];
};

#endif //POOL_THREADCACHE_H
