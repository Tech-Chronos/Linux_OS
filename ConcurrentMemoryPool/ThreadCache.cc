//
// Created by dsj on 2026/3/14.
//

#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t size)
{
    size_t obj_size = SizeClass::RoundUp(size);
    size_t index = SizeClass::Index(obj_size);

    if (!_freelists[index].Empty())
    {
        return _freelists[index].Pop();
    }
    else
    {
        return FetchFromCentralCache(size);
    }
}

void ThreadCache::DeAllocate(void* ptr, size_t size)
{
    assert(ptr);
    assert(size < MAX_BYTES);
    // 找自由链表桶，进行插入
    size_t obj_size = SizeClass::RoundUp(size);
    size_t index = SizeClass::Index(obj_size);

    _freelists[index].Push(ptr);
}

// 当自由链表中的不够了，就要到中心缓存拿
void* ThreadCache::FetchFromCentralCache(size_t bytes)
{
    size_t obj_size = SizeClass::RoundUp(bytes);
    // 拿到桶号
    size_t index = SizeClass::Index(obj_size);

    // 慢启动算法
    int distribute_nums = std::min(SizeClass::NumMoveSize(bytes), _freelists[index].GetMaxSize());

    if (distribute_nums == _freelists[index].GetMaxSize())
    {
        ++_freelists[index].GetMaxSize();
    }

    void* start = nullptr;
    void* end = nullptr;

    CentralCache::GetSinglton()->FetchRangeObj(start, end, distribute_nums, bytes);

}

