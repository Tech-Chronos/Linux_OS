//
// Created by dsj on 2026/3/17.
//

#include "PageCache.h"

Span* PageCache::NewSpan(size_t npage)
{
    if (npage > NPAGE_LISTS - 1)
    {
        void* ptr = SystemAlloc(npage);
        Span* span = _obj_pool.New();
        span->_free_lists = nullptr;
        span->_prev = nullptr;
        span->_next = nullptr;
        span->_use_count = 0;
        span->_obj_size = 0;
        span->_is_used = false;
        span->_page_id = (PAGEID)ptr >> PAGE_SHIFT; // 计算出页号
        span->_npage = npage;

        for (PAGEID id = 0; id < npage; ++id)
        {
            _id_span_map.set(span->_page_id + id, span);
        }

        return span;
    }
    // page cache 中，第 napage 个桶不为空，将这个桶头删
    if (!_span_lists[npage].Empty())
    {
        Span* span = _span_lists[npage].Pop_Front();

        // 先对这个span的首尾映射删除，然后再插入
        _id_span_map.erase(span->_page_id);
        _id_span_map.erase(span->_page_id + span->_npage - 1);

        for (int id = 0; id < npage; ++id)
        {
            _id_span_map.set(span->_page_id + id, span);
        }
        return span;
    }

    // 第 napage 个桶为空，向后找比他大的page，看一下，进行切割
    for (size_t i = npage + 1; i < NPAGE_LISTS; ++i)
    {
//        if (i == 128)
//            int x = 0;
        if (!_span_lists[i].Empty())
        {
            // 切割
            Span* kSpan = _obj_pool.New();
            kSpan->_free_lists = nullptr;
            kSpan->_prev = nullptr;
            kSpan->_next = nullptr;
            kSpan->_use_count = 0;
            kSpan->_obj_size = 0;
            kSpan->_is_used = false;
            // 头删一个span
            Span* nSpan = _span_lists[i].Pop_Front();

            kSpan->_page_id = nSpan->_page_id;
            kSpan->_npage = npage;

            nSpan->_page_id += npage;
            nSpan->_npage -= npage;

            _span_lists[nSpan->_npage].Push_Front(nSpan);

            // 没被切走的页，是整块的，只要记录开始和结尾的 pageid 和 span 的映射关系
            _id_span_map.set(nSpan->_page_id, nSpan);
            _id_span_map.set(nSpan->_page_id + nSpan->_npage - 1, nSpan);

            // 被切走的页，每一页都要记录，因为central cache合并的时候要知道每一小块的页属于哪个 span
            // 可能被分给了不同的 thread cache
            for (PAGEID id = 0; id < npage; ++id)
            {
                _id_span_map.set(kSpan->_page_id + id, kSpan);
            }
            return kSpan;
        }
    }
    // 到这里说明 npage 后面的桶都没有，就向堆申请
    // 向系统申请 NPAGE 页
    Span* span = _obj_pool.New();
    span->_free_lists = nullptr;
    span->_prev = nullptr;
    span->_next = nullptr;
    span->_use_count = 0;
    span->_obj_size = 0;
    span->_is_used = false;
    void* ptr= SystemAlloc(NPAGE_LISTS);
    // 设置span的id和页码
    span->_page_id = ((PAGEID)ptr) >> PAGE_SHIFT;
    span->_npage = NPAGE_LISTS - 1;

    // 插入到 list 中
    _span_lists[NPAGE_LISTS - 1].Push_Front(span);

    return NewSpan(npage);
}

Span* PageCache::PageToSpan(void* obj)
{
    //std::unique_lock<std::mutex> lock(_page_mutex);
    PAGEID id = ((PAGEID)obj) >> PAGE_SHIFT;

    Span* span = reinterpret_cast<Span*>(_id_span_map.get(id));
    if (span != nullptr)
    {
        return span;
    }
    else
    {
        assert(false);
        return nullptr;
    }
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
    if (span->_npage > NPAGE_LISTS - 1)
    {
        for (PAGEID id = 0; id < span->_npage; ++id)
        {
            _id_span_map.erase(span->_page_id + id);
        }

        void* ptr = (void*)(span->_page_id << PAGE_SHIFT);
        SystemFree(ptr, span->_npage);
        //delete span;
        _obj_pool.Delete(span);

        return;
    }
    // 因为分配给central cache中的span每一页都在 map page 中有映射
    // 但是回到page cache中只有首尾有映射，所以要删除中间的
    for (int id = 0; id < span->_npage; ++id)
    {
        PAGEID page_id = span->_page_id + id;
        _id_span_map.erase(page_id);
    }
    // 向前合并
    while (true)
    {
        // 退出条件：1. 不存在这个页号 2. 前面的这个页号在使用，没有归还 3. 整个页的大小超过了 NPAGE - 1
        PAGEID id = span->_page_id - 1;
        Span* prev_span = reinterpret_cast<Span*>(_id_span_map.get(id));
        // 不存在这个页号
        if (prev_span == nullptr)
        {
            break;
        }
        // 前面的这个页号在使用，没有归还
        if (prev_span->_is_used)
        {
            break;
        }
        // 整个页的大小超过了 NPAGE - 1
        if (span->_npage + prev_span->_npage > NPAGE_LISTS - 1)
        {
            break;
        }

        span->_page_id = prev_span->_page_id;
        span->_npage += prev_span->_npage;

        _span_lists[prev_span->_npage].Erase(prev_span);

        _id_span_map.erase(prev_span->_page_id);
        _id_span_map.erase(prev_span->_page_id + prev_span->_npage - 1);

        //delete prev_span;
        _obj_pool.Delete(prev_span);
    }

    // 向后合并，一样的逻辑
    while (true)
    {
        PAGEID id = span->_page_id + span->_npage;

        Span* next_span = reinterpret_cast<Span*>(_id_span_map.get(id));
        if (next_span == nullptr)
        {
            break;
        }
        if (next_span->_is_used)
        {
            break;
        }
        if (span->_npage + next_span->_npage > NPAGE_LISTS - 1)
        {
            break;
        }

        span->_npage += next_span->_npage;

        _span_lists[next_span->_npage].Erase(next_span);

        _id_span_map.erase(next_span->_page_id);
        _id_span_map.erase(next_span->_page_id + next_span->_npage - 1);


        //delete next_span;
        _obj_pool.Delete(next_span);
    }

    // 在page map 中修改
    _span_lists[span->_npage].Push_Front(span);

    span->_is_used = false;
    _id_span_map.set(span->_page_id, span);
    _id_span_map.set(span->_page_id + span->_npage - 1, span);
}
