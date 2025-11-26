#include "test_thread.hpp"

int ticketnum = 10000;

// 多线程的问题
void BuyTicket(const std::string& name)
{
    while (ticketnum > 0)
    {
        usleep(1000);
        printf("I am the %s thread, I have bought the %d ticket\n", name.c_str(), ticketnum);
        --ticketnum;
    }
}

int main()
{
    std::vector<mythread> threads;

    for (int i = 0; i < 5; ++i)
    {
        std::string name = "thread ["  + std::to_string(i + 1) + "]";
        threads.emplace_back(name, BuyTicket);
        //sleep(1);
    }

    for (auto& thread : threads)
    {
        thread.Start();
    }

    for (auto& thread : threads)
    {
        thread.Join();
    }
    return 0;
}




//const int num = 5;

// void task(const std::string& name)
// {
//     while (true)
//     {
//         std::cout << "My name is " << name  << ", I am doing the task!" << std::endl;
//         sleep(1);
//     }
// }

// int main()
// {
//     // 创建多个线程
//     std::vector<mythread> threads;

//     for (int i = 0; i < num; ++i)
//     {
//         std::string name = "thread ["  + std::to_string(i + 1) + "]";
//         threads.emplace_back(name, task);
//         sleep(1);
//     }

//     for (auto& thread : threads)
//     {
//         thread.Start();
//     }

//     sleep(5);

//     for (auto& thread : threads)
//     {
//         thread.Stop();
//     }

//     sleep(2);

//     for (auto& thread : threads)
//     {
//         thread.Join();
//     }

//     // 只创建一个线程
//     // mythread thread("thread [1]", task);

//     // thread.Start();

//     // sleep(5);
//     // thread.Stop();
//     // sleep(2);

//     // thread.Join();
//     return 0;
// }