#include "ThreadPool.hpp"

int main()
{
    srand(time(nullptr) ^ getpid());
    ThreadPool<task_t> thp(5);

    thp.Init();

    thp.Start();

    int cnt = 10;
    while (cnt)
    {
        int rand_suffix = rand() % 4;
        thp.Enqueue(tasks[rand_suffix]);
        std::cout << cnt-- << std::endl;
        sleep(1);
    }

    thp.Stop();

    return 0;
}

// 这是一个 测试 线程的代码
// int main()
// {
//     srand(time(nullptr) ^ getpid());

//     std::string name = "thread [1]";

//     while (true)
//     {
//         int rand_suffix = rand() % 4;

//         Thread<task_t> thread(name, tasks[rand_suffix]);
//         thread.Start();
//         thread.Join();
//         sleep(1);
//     }

//     return 0;
// }