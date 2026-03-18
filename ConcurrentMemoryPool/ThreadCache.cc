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

// 当自由链表中的不够了，就要到中心缓存拿
void* ThreadCache::FetchFromCentralCache(size_t bytes)
{
    size_t obj_size = SizeClass::RoundUp(bytes);
    // 拿到桶号
    size_t index = SizeClass::Index(obj_size);

    // 慢启动算法
    int distribute_nums = std::min(SizeClass::NumMoveSize(obj_size), _freelists[index].GetMaxSize());

    if (distribute_nums == _freelists[index].GetMaxSize())
    {
        ++_freelists[index].GetMaxSize();
    }

    void* start = nullptr;
    void* end = nullptr;

    size_t actual_num = CentralCache::GetSingleton()->FetchRangeObj(start, end, distribute_nums, obj_size);

    assert(actual_num >= 1);

    // 如果只有一个，直接返回即可
    if (actual_num == 1)
        return start;

    // 如果有多个，返回一个，其他的插入桶里面
    _freelists[index].Push_Range(NextObj(start), end, actual_num - 1);
    return start;
}

void ThreadCache::DeAllocate(void* ptr, size_t size)
{
    assert(ptr);
    assert(size < MAX_BYTES);
    // 找自由链表桶，进行插入
    size_t obj_size = SizeClass::RoundUp(size);
    size_t index = SizeClass::Index(obj_size);

    _freelists[index].Push(ptr);

    if (_freelists[index].Size() >= _freelists[index].GetMaxSize())
    {
        ListTooLong(_freelists[index], obj_size);
    }
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
    void* start = nullptr;
    void* end = nullptr;

    list.Pop_Range(start, end, list.GetMaxSize());

    CentralCache::GetSingleton()->ReleaseListToSpans(start, size);
}
