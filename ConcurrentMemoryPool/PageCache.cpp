//
// Created by dsj on 2026/3/17.
//

#include "PageCache.h"

Span* PageCache::NewSpan(size_t npage)
{
    // page cache 中，第 napage 个桶不为空，将这个桶头删
    if (!_span_lists[npage].Empty())
    {
        return _span_lists[npage].Pop_Front();
    }

    // 第 napage 个桶为空，向后找比他大的page，看一下，进行切割
    for (size_t i = npage + 1; i < NPAGE_LISTS; ++i)
    {
//        if (i == 128)
//            int x = 0;
        if (!_span_lists[i].Empty())
        {
            // 切割
            Span* kSpan = new Span;
            // 头删一个span
            Span* nSpan = _span_lists[i].Pop_Front();

            kSpan->_page_id = nSpan->_page_id;
            kSpan->_npage = npage;

            nSpan->_page_id += npage;
            nSpan->_npage -= npage;

            _span_lists[nSpan->_npage].Push_Front(nSpan);
            return kSpan;
        }
    }

    // 到这里说明 napage 后面的桶都没有，就向堆申请
    // 向系统申请 NPAGE 页
    Span* span = new Span;
    void* ptr= SystemAlloc(NPAGE_LISTS);
    // 设置span的id和页码
    span->_page_id = ((PAGEID)ptr) >> PAGE_SHIFT;
    span->_npage = NPAGE_LISTS - 1;

    // 插入到 list 中
    _span_lists[NPAGE_LISTS - 1].Push_Front(span);

    return NewSpan(npage);
}
