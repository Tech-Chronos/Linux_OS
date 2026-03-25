//
// Created by dsj on 2026/3/24.
//

#include "PageCache.h"

ObjectPool<PageCache> PageCache::_pool;

ObjectPool<Span> PageCache::pool;

namespace
{
    void MapSpanRange(TCMalloc_PageMap3<64 - PAGE_SHIFT>& tree, PAGEID start, size_t pages, Span* span)
    {
        for (PAGEID page = start; page < start + pages; ++page)
        {
            tree.set(page, span);
        }
    }

    void MapFreeSpan(TCMalloc_PageMap3<64 - PAGE_SHIFT>& tree, Span* span)
    {
        MapSpanRange(tree, span->_id, span->_n_pages, nullptr);
        tree.set(span->_id, span);
        tree.set(span->_id + span->_n_pages - 1, span);
    }
}

PageCache *PageCache::Singleton()
{
    static PageCache* instance = (PageCache*)_pool.New(sizeof(PageCache));
    //static PageCache* instance = new PageCache;
    return instance;
}

// 被分走之后 一定要将 span 的 isused 改成 true
Span *PageCache::GetNewSpan(int page_num)
{
    assert(page_num < NPAGE_LIST);
    // 1. 先判断 第page_num个桶是否有span
    if (!_span_lists[page_num].empty())
    {
        Span* span = _span_lists[page_num].pop_back();
        span->_is_used = true;
        // 分配出去的每一页都要在哈希表中有映射
        MapSpanRange(_radix_tree, span->_id, span->_n_pages, span);
        return span;
    }
    else
    {
        // 如果当前的桶没有，就从后面的桶找，看是否有比他大的页，如果有就切割
        for (int i = page_num + 1; i < NPAGE_LIST; ++i)
        {
            if (!_span_lists[i].empty())
            {
                Span* npage = _span_lists[i].pop_back();

                Span* kpage = (Span*)pool.New(sizeof(Span));
                //Span* kpage = new Span;
                kpage->_n_pages = page_num;
                kpage->_id = npage->_id;
                kpage->_is_used = true;

                npage->_id += page_num;
                npage->_n_pages -= page_num;
                npage->_is_used = false;

                // 仅将切分后的剩余页挂回 PageCache，返回申请得到的 kpage。
                _span_lists[npage->_n_pages].push_front(npage);

                // 分配出去的每一页都要在 id_map 中有对应的映射，映射到 span
                // 也就是说 每一个页号 都有对应的 span
                // 而页号可以通过 addr >> PAGESHIFT 计算得到
                // 因此归还的时候就可以直接通过 地址 -> 页号 -> span
                MapSpanRange(_radix_tree, kpage->_id, kpage->_n_pages, kpage);
                MapFreeSpan(_radix_tree, npage);

                return kpage;
            }
        }
    }

    // 走到这里说明没有，向系统申请
    Span* new_span = (Span*)pool.New(sizeof(Span));
    //Span* new_span = new Span;
    new_span->_n_pages = NPAGE_LIST - 1;
    new_span->_is_used = false;

    void* ptr = SystemAlloc(new_span->_n_pages);
    new_span->_id = ((PAGEID)ptr >> PAGE_SHIFT);

    _span_lists[new_span->_n_pages].push_front(new_span);

    return GetNewSpan(page_num);
}

Span *PageCache::AddrToSpan(void *addr)
{
    // 这里不用加锁是提升效率最高的
    //std::unique_lock<std::mutex> lock(_mtx);
    PAGEID id = ((PAGEID)addr >> PAGE_SHIFT);

    //auto it = _id_span_map.find(id);

    auto it = (Span*)_radix_tree.get(id);
    if (it == nullptr)
        return nullptr;

    //return it->second;
    return it;
}

void PageCache::ReleaseSpanToPageCache(Span *span)
{
    std::unique_lock<std::mutex> lock(_mtx);

    // 向前合并
    while (true)
    {
        Span* prev_span = nullptr;
        //auto it = _id_span_map.find(span->_id - 1);
        auto it = (Span*)_radix_tree.get(span->_id - 1);
        if (it != nullptr)
            prev_span = it;

        if (prev_span == nullptr)
            break;
        if (prev_span->_is_used)
            break;
        if (prev_span->_n_pages + span->_n_pages > NPAGE_LIST - 1)
            break;

        size_t old_id = span->_id;

        span->_id = prev_span->_id;
        span->_n_pages += prev_span->_n_pages;

        // 将 prev_span 从list中删除，按照页数
        _span_lists[prev_span->_n_pages].erase(prev_span);

        // 两个合并到一起，只要删除prev的尾部和span的头部在hash的映射 -> 因为只要记录头尾的
        // 按照页码
        //_id_span_map.erase(prev_span->_id + prev_span->_n_pages - 1);
        MapSpanRange(_radix_tree, prev_span->_id, prev_span->_n_pages, nullptr);
        MapSpanRange(_radix_tree, old_id, span->_n_pages - prev_span->_n_pages, nullptr);

        //delete prev_span;
        pool.Delete(prev_span);
    }

    // 向后合并
    while (true)
    {
        Span* next_span = nullptr;
        //auto it = _id_span_map.find(span->_id + span->_n_pages);
        auto it = (Span*)_radix_tree.get(span->_id + span->_n_pages);
        if (it != nullptr)
            next_span = it;

        if (next_span == nullptr)
            break;
        if (next_span->_is_used)
            break;
        if (next_span->_n_pages + span->_n_pages > NPAGE_LIST - 1)
            break;

        size_t old_pages = span->_n_pages;
        // 更新span的_n_pages
        span->_n_pages += next_span->_n_pages;

        // 按照 页数删除
        _span_lists[next_span->_n_pages].erase(next_span);

        //_id_span_map.erase(span->_id + old_pages - 1);
        MapSpanRange(_radix_tree, next_span->_id, next_span->_n_pages, nullptr);
        MapSpanRange(_radix_tree, span->_id, old_pages, nullptr);


        //delete next_span;
        pool.Delete(next_span);
    }
    _span_lists[span->_n_pages].push_front(span);
    span->_is_used = false;

    //_id_span_map[span->_id] = span;
    MapFreeSpan(_radix_tree, span);

}
