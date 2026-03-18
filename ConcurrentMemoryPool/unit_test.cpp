#include "ConCurrent.h"

void Alloc1()
{
    for (int i = 0; i < 5; ++i)
    {
        ConcurrentMalloc(110 * 1024);
    }
}

void Alloc2()
{
    for (int i = 0; i < 5; ++i)
    {
        ConcurrentMalloc(1);
    }
}


void TLSTest()
{
    std::thread t1(Alloc1);
    t1.join();
    std::thread t2(Alloc2);
    t2.join();
}

void TestConCurrentAlloc1()
{
    void* p1 = ConcurrentMalloc(6);
    void* p2 = ConcurrentMalloc(8);
    void* p3 = ConcurrentMalloc(1);
    void* p4 = ConcurrentMalloc(7);
    void* p5 = ConcurrentMalloc(8);

    cout << p1 << endl;
    cout << p2 << endl;
    cout << p3 << endl;
    cout << p4 << endl;
    cout << p5 << endl;
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


int main()
{
    TestConCurrentAlloc2();

    return 0;
}
