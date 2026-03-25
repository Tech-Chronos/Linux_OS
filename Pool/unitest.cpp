#include "Concurrent.h"

void SingleThreadTest()
{
    std::cout << "Single Thread Test Begin..." << std::endl;

    for (int i = 0; i < 100000; ++i)
    {
        void* ptr = Alloc(64);
        Free(ptr);
    }

    std::cout << "Single Thread Test End..." << std::endl;
}

void ConcurrentAlloc(int n, int size)
{
    std::vector<void*> v;

    for (int i = 0; i < n; ++i)
    {
        void* ptr = Alloc(size);
        v.push_back(ptr);
    }

    for (auto e : v)
    {
        Free(e);
    }

}

void MultiThreadTest()
{
    std::cout << "Multi Thread Test Begin..." << std::endl;

    std::vector<std::thread> threads;

    int thread_num = 10;

    for (int i = 0; i < thread_num; ++i)
    {
        threads.emplace_back(ConcurrentAlloc, 100000, 64);
    }

    for (auto& t : threads)
    {
        t.join();
    }

    std::cout << "Multi Thread Test End..." << std::endl;
}

void StressTest()
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; ++i)
    {
        threads.emplace_back([]()
                             {
                                 for (int i = 0; i < 1000000; ++i)
                                 {
                                     int size = rand() % 512 + 1;
                                     void* p = Alloc(size);
                                     Free(p);
                                 }
                             });
    }

    for (auto& t : threads)
    {
        t.join();
    }
}

//int main()
//{
//    SingleThreadTest();
//
//    MultiThreadTest();
//    StressTest();
//    return 0;
//}
