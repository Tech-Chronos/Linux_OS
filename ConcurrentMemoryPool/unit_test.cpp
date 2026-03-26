#include "ConCurrent.h"

void Alloc1()
{
    std::vector<void*> v;
    for (int i = 0; i < 17; ++i)
    {
        void *ret = ConcurrentMalloc(110 * 1024);
        v.emplace_back(ret);
    }
    for (auto e : v)
    {
        ConcurrentDelete(e);
    }
}

void Alloc2()
{
    std::vector<void*> v;
    for (int i = 0; i < 15; ++i)
    {
        void* ret = ConcurrentMalloc(7);
        v.push_back(ret);
    }

    for (auto e : v)
    {
        ConcurrentDelete(e);
    }
}


void TLSTest()
{
    std::thread t1(Alloc1);
    std::thread t2(Alloc2);
    t1.join();
    t2.join();
}

void TestConCurrentAlloc1()
{
    void* p1 = ConcurrentMalloc(6);
    void* p2 = ConcurrentMalloc(8);
    void* p3 = ConcurrentMalloc(1);
    void* p4 = ConcurrentMalloc(7);
    void* p5 = ConcurrentMalloc(8);
    void* p6 = ConcurrentMalloc(2);
    void* p7 = ConcurrentMalloc(5);

    ConcurrentDelete(p1);
    ConcurrentDelete(p2);
    ConcurrentDelete(p3);
    ConcurrentDelete(p4);
    ConcurrentDelete(p5);
    ConcurrentDelete(p6);
    ConcurrentDelete(p7);
}

void TestConCurrentAlloc2()
{
    for (int i = 0; i < 512; ++i)
    {
        void* p1 = ConcurrentMalloc(6);
        cout << p1 << endl;
    }

    void* p2 = ConcurrentMalloc(6);
    cout << p2 << endl;
}

void TestConCurrentDeAlloc()
{
    void* p1 = ConcurrentMalloc(6);
    ConcurrentDelete(p1);
}


//int main()
//{
//    TLSTest();
//    //TestConCurrentDeAlloc();
//
//    return 0;
//}
