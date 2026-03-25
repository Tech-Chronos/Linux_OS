//
// Created by dsj on 2026/3/23.
//

#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(int size)
{
    // 计算出偏移量和在那个桶
    int align_size = SizeClass::RoundUp(size);
    int index = SizeClass::Index(align_size);

    if (!_free_lists[index].Empty())
    {
        return _free_lists[index].pop_front();
    }
    else
    {
        return FetchFromCentralCache(align_size);
    }
}

void* ThreadCache::FetchFromCentralCache(int size)
{
    // 1. 计算出需要向 central cache 中获取多少个内存块
    int index = SizeClass::Index(size);
    int obj_num = std::min(SizeClass::NumsFromCentralCache(size), _free_lists[index].MaxSize());

    if (obj_num == _free_lists[index].MaxSize())
        ++_free_lists[index].MaxSize();

    // 2. 申请
    void* start = nullptr, *end = nullptr;

    int actual_num = CentralCache::Singleton()->FetchRangeObj(start, end, size, obj_num);

    // 3. 申请到的 actual_num 个，返回一个给上层，其他的插入到freelist中
    if (actual_num > 1)
        _free_lists[index].push_range_obj(NextObj(start), end, actual_num - 1);

    return start;
}

void ThreadCache::Deallocate(void *ptr, int size)
{
    // 如果thread cache中的链表过长，就要归还给central cache中的span，给其他线程来用
    // 链表过长的标准是超过了阈值max size；
    size = SizeClass::RoundUp(size);
    int index = SizeClass::Index(size);

    _free_lists[index].push_front(ptr);

    // 超过了阈值，就要将内存块返回给central cache
    if (_free_lists[index].Size() > _free_lists[index].MaxSize())
    {
        ListTooLong(_free_lists[index], size);
    }
}

void ThreadCache::ListTooLong(FreeList &list, int size)
{
    void* start = nullptr, *end = nullptr;

    // 自己可以留一点，不要全部归还
    list.pop_range_obj(start, end, list.MaxSize());

    // 归还给central cache
    CentralCache::Singleton()->ReleaseToSpan(start, end, size, list.MaxSize());
}


