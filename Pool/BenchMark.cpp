//
// Created by dsj on 2026/3/25.
//
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include "Concurrent.h"

//using namespace std;
//
//static const int nworks = 16;
//static const int rounds = 2000000;
//
//// malloc 测试
//void BenchmarkMalloc()
//{
//    vector<void*> v;
//    v.reserve(rounds);
//
//    std::default_random_engine eng;
//    std::uniform_int_distribution<int> dist(1, 512);
//
//    auto begin = std::chrono::high_resolution_clock::now();
//
//    for (int i = 0; i < rounds; ++i)
//    {
//        size_t size = dist(eng) ;
//        void* p = malloc(size);
//        v.push_back(p);
//    }
//
//    for (auto e : v)
//    {
//        free(e);
//    }
//
//    auto end = std::chrono::high_resolution_clock::now();
//
//    cout << "malloc thread cost: "
//         << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
//         << " ms" << endl;
//}
//
//// mini tcmalloc 测试
//void BenchmarkConcurrentAlloc()
//{
//    vector<void*> v;
//    v.reserve(rounds);
//
//    std::default_random_engine eng;
//    std::uniform_int_distribution<int> dist(1, 512);
//
//    auto begin = std::chrono::high_resolution_clock::now();
//
//    for (int i = 0; i < rounds; ++i)
//    {
//        size_t size = dist(eng);
//        void* p = Alloc(size);
//        v.push_back(p);
//    }
//
//    for (auto e : v)
//    {
//        Free(e);
//    }
//
//    auto end = std::chrono::high_resolution_clock::now();
//
//    cout << "mini tcmalloc thread cost: "
//         << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
//         << " ms" << endl;
//}
//
//
//// 多线程测试 malloc
//void MultiThreadMalloc()
//{
//    vector<thread> threads;
//
//    auto begin = chrono::high_resolution_clock::now();
//
//    for (int i = 0; i < nworks; ++i)
//    {
//        threads.emplace_back(BenchmarkMalloc);
//    }
//
//    for (auto& t : threads)
//    {
//        t.join();
//    }
//
//    auto end = chrono::high_resolution_clock::now();
//
//    cout << "MultiThread malloc total cost: "
//         << chrono::duration_cast<chrono::milliseconds>(end - begin).count()
//         << " ms" << endl;
//}
//
//
//// 多线程测试 mini tcmalloc
//void MultiThreadConcurrentAlloc()
//{
//    vector<thread> threads;
//
//    auto begin = chrono::high_resolution_clock::now();
//
//    for (int i = 0; i < nworks; ++i)
//    {
//        threads.emplace_back(BenchmarkConcurrentAlloc);
//    }
//
//    for (auto& t : threads)
//    {
//        t.join();
//    }
//
//    auto end = chrono::high_resolution_clock::now();
//
//    cout << "MultiThread mini tcmalloc total cost: "
//         << chrono::duration_cast<chrono::milliseconds>(end - begin).count()
//         << " ms" << endl;
//}
//
//
//int main()
//{
//    cout << "=============================" << endl;
//    cout << "MultiThread malloc test" << endl;
//    cout << "=============================" << endl;
//
//    MultiThreadMalloc();
//
//    cout << endl;
//
//    cout << "=============================" << endl;
//    cout << "MultiThread mini tcmalloc test" << endl;
//    cout << "=============================" << endl;
//
//    MultiThreadConcurrentAlloc();
//
//    return 0;
//}

void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
    std::vector<std::thread> vthread(nworks);
    std::atomic<size_t> malloc_costtime = 0;
    std::atomic<size_t> free_costtime = 0;
    for (size_t k = 0; k < nworks; ++k)
    {
        vthread[k] = std::thread([&, k]() {
            std::vector<void*> v;
            v.reserve(ntimes);
            for (size_t j = 0; j < rounds; ++j)
            {
                size_t begin1 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    //v.push_back(malloc(16));
                    v.push_back(malloc((16 + i) % 8192 + 1));
                }
                size_t end1 = clock();
                size_t begin2 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    free(v[i]);
                }
                size_t end2 = clock();
                v.clear();
                malloc_costtime += (end1 - begin1);
                free_costtime += (end2 - begin2);
            }
        });
    }
    for (auto& t : vthread)
    {
        t.join();
    }
    printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n",
           nworks, rounds, ntimes, malloc_costtime.load());
    printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n",
           nworks, rounds, ntimes, free_costtime.load());
    printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n",
           nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);
}

void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
    std::vector<std::thread> vthread(nworks);
    std::atomic<size_t> malloc_costtime = 0;
    std::atomic<size_t> free_costtime = 0;
    for (size_t k = 0; k < nworks; ++k)
    {
        vthread[k] = std::thread([&]() {
            std::vector<void*> v;
            v.reserve(ntimes);
            for (size_t j = 0; j < rounds; ++j)
            {
                size_t begin1 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    //v.push_back(Alloc(16));
                    v.push_back(Alloc((16 + i) % 8192 + 1));
                }
                size_t end1 = clock();
                size_t begin2 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    Free(v[i]);
                }
                size_t end2 = clock();
                v.clear();
                malloc_costtime += (end1 - begin1);
                free_costtime += (end2 - begin2);
            }
        });
    }
    for (auto& t : vthread)
    {
        t.join();
    }
    printf("%u个线程并发执行%u轮次，每轮次concurrent alloc %u次: 花费：%u ms\n",
           nworks, rounds, ntimes, malloc_costtime.load());
    printf("%u个线程并发执行%u轮次，每轮次concurrent dealloc %u次: 花费：%u ms\n",
           nworks, rounds, ntimes, free_costtime.load());
    printf("%u个线程并发concurrent alloc&dealloc %u次，总计花费：%u ms\n",
           nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);
}

int main()
{
    size_t n = 10000;
    cout << "==========================================================" <<
         endl;
    BenchmarkConcurrentMalloc(n, 4, 10);
    cout << endl << endl;
    BenchmarkMalloc(n, 4, 10);
    cout << "==========================================================" <<
         endl;
    return 0;
}
