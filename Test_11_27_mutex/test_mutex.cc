#include "Thread.hpp"

static int num = 10;

static int tickets_num = 10000;

void BuyingTickets(const threaddata& _td)
{
    while (true)
    {
        //pthread_mutex_t* mutex = _td.getmutex();

        //std::cout << "mutex address: " << mutex << std::endl;
        //pthread_mutex_lock(mutex);
        LockGuard lock(_td.getmutex());
        if (tickets_num > 0)
        {
            usleep(100);
            std::cout << _td.getname() << " has bought number: "<< tickets_num << " ticket!" << std::endl;
            --tickets_num;
            //pthread_mutex_unlock(mutex);
        }
        else
        {
            //pthread_mutex_unlock(mutex);
            break;
        }
    }
}

int main()
{
    std::vector<mythread> threads;

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, nullptr);

    for (int i = 0; i < num; ++i)
    {
        std::string name = "thread [" + std::to_string(i + 1) + "]"; 
        
        threaddata td(&mutex, name);

        threads.emplace_back(td, BuyingTickets);
    }


    sleep(1);

    for (auto& thread : threads)
    {
        thread.Start();
    }

    sleep(1);

    for (auto& thread : threads)
    {
        thread.Join();
    }

    return 0;
}

// void BuyingTickets(const std::string& name)
// {
//     while (true)
//     {
//         if (tickets_num > 0)
//         {
//             usleep(1000);
//             std::cout << name << " has bought number: "<< tickets_num << " ticket!" << std::endl;
//             --tickets_num;
//         }
//         else
//         {
//             break;
//         }
//     }
// }

// int main()
// {
//     std::vector<mythread> threads;
//     for (int i = 0; i < num; ++i)
//     {
//         std::string name = "thread [" + std::to_string(i + 1) + "]"; 
//         threads.emplace_back(name, BuyingTickets);
//     }

//     sleep(1);

//     for (auto& thread : threads)
//     {
//         thread.Start();
//     }

//     sleep(1);

//     for (auto& thread : threads)
//     {
//         thread.Join();
//     }


//     return 0;
// }