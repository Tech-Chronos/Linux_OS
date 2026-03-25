//
// Created by dsj on 2026/3/23.
//

#include "CentralCache.h"
#include "PageCache.h"
ObjectPool<CentralCache> CentralCache::_pool;

CentralCache* CentralCache::Singleton()
{
    static CentralCache* instance = (CentralCache*)_pool.New(sizeof(CentralCache));
    //static CentralCache* instance = new CentralCache;
    return instance;
}

int CentralCache::FetchRangeObj(void*& start, void*& end, int size, int obj_num)
{
    int index = SizeClass::Index(size);
    // 访问加桶锁
    std::unique_lock<std::mutex> lock(_span_lists[SizeClass::Index(size)]._mtx);
    // 获取桶中的一个span
    Span* span = GetOneSpan(size);
    // 如果CentralCache中没有，就要从page cache中获取
    if (span == nullptr)
    {
        lock.unlock();
        // 挂载到central cache的桶中
        span = FetchFromPageCache(size);
        //span->_is_used = true;
        lock.lock();
        _span_lists[index].push_front(span);
    }
    assert(span->_free_list);
    // 将span分割出来的几个内存块分给上层
    start = span->_free_list;
    end = start;

    int actual_num = 1;

    while (--obj_num && NextObj(end))
    {
        end = NextObj(end);
        ++actual_num;
    }

    span->_use_count += actual_num;
    // 更新span的头节点
    span->_free_list = NextObj(end);

    NextObj(end) = nullptr;
    return actual_num;
}


Span *CentralCache::GetOneSpan(int size)
{
    int index = SizeClass::Index(size);
    Span* it = _span_lists[index].begin();

    while (it != _span_lists[index].end())
    {
        if (it->_free_list != nullptr)
            return it;
        it = it->_next;
    }
    return nullptr;
}

// central cache 中没有对应的值，要从 page cache 中申请，size 是每个内存块的大小
Span* CentralCache::FetchFromPageCache(int size)
{
    int apply_page_num = SizeClass::NumsFronPageCache(size);

    // 加全局锁
    PageCache::Singleton()->_mtx.lock();

    // 拿到了之后要进行分割 freelist
    Span* new_span = PageCache::Singleton()->GetNewSpan(apply_page_num);
    assert(new_span);
    // 从page cache中拿到了span可以立即解锁，因为拿到了这个span是线程自己私有的了
    PageCache::Singleton()->_mtx.unlock();

    // 根据页号算出地址
    char* start = (char*)(new_span->_id << PAGE_SHIFT);
    new_span->_free_list = start;
    char* end = start + (1 << PAGE_SHIFT) * new_span->_n_pages;

    new_span->_obj_size = size;
    // 切割freelist
    void* cur = start;
    while ((char*)cur + size < end)
    {
        void* next = (char*)cur + size;
        NextObj(cur) = next;
        cur = next;
    }
    // 切割完成之后，要对最后一个置空
    NextObj(cur) = nullptr;

    return new_span;
}

void CentralCache::ReleaseToSpan(void *&start, void *&end, int size, int return_size)
{
    int index = SizeClass::Index(size);
    //_span_lists[index]._mtx.lock();
    std::unique_lock<std::mutex> lock(_span_lists[index]._mtx);
    void* cur = start;
    while (cur)
    {
        // 先记录next
        void* next = NextObj(cur);

        // 通过freelist的地址找到对应的span
        Span* ret_span = PageCache::Singleton()->AddrToSpan(cur);
        assert(ret_span);

        // 在span中插入这个内存块，进行头插
        NextObj(cur) = ret_span->_free_list;
        ret_span->_free_list = cur;

        // 归还要更新usecount
        --ret_span->_use_count;

        // 如果更新到0，说明没有内存块给thread cache用了，要把这个内存块归还给page cache
        if (ret_span->_use_count == 0)
        {
            _span_lists[index].erase(ret_span);
            lock.unlock();
            PageCache::Singleton()->ReleaseSpanToPageCache(ret_span);
            lock.lock();
        }
        cur = next;
    }
}



