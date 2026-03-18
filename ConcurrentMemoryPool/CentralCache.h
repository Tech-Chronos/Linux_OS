//
// Created by dsj on 2026/3/16.
//

#ifndef CONCURRENTMEMORYPOOL_CENTRALCACHE_H
#define CONCURRENTMEMORYPOOL_CENTRALCACHE_H
#include "Common.h"
#include "PageCache.h"

class CentralCache
{
private:
    CentralCache() = default;

    Span* FetchOneSpan(SpanList&, size_t);
public:
    CentralCache(const CentralCache&) = delete;
    CentralCache& operator=(const CentralCache&) = delete;

    // 单例，对于所有的 thread cache 只有一个 central cache
    static CentralCache* GetSingleton()
    {
        if (_central_singlton == nullptr)
        {
            std::unique_lock<std::mutex> _lock(_singlton_mutex);
            if (_central_singlton == nullptr)
            {
                _central_singlton = new CentralCache;
            }
        }
        return _central_singlton;
    }

    size_t FetchRangeObj(void*& start, void*& end, size_t batch_num, size_t size);

    void ReleaseListToSpans(void* start, size_t size);
private:
    SpanList _span_lists[NFREE_LISTS];
    static std::mutex _singlton_mutex;
    static CentralCache* _central_singlton;
};

#endif //CONCURRENTMEMORYPOOL_CENTRALCACHE_H
