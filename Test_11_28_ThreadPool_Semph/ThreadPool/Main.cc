#include "Thread.hpp"
#include "ThreadPool.hpp"
#include "Task.hpp"

int main()
{
    srand(time(nullptr) ^ getpid());
    ThreadPool<task_t> pool(5);

    pool.Init();

    pool.Start();

    while (true)
    {
        size_t suffix = rand() % 4;
        sleep(1);
        pool.Enqueue(tasks[suffix]);
        sleep(1);
    }
    
    return 0;
}