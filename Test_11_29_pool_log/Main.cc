#include "ThreadPool.hpp"
#include "Log.hpp"
#include "Task.hpp"

int main()
{
    srand(time(nullptr) ^ getpid());
    //ThreadPool<task_t> thp(5);

    ThreadPool<task_t>* tp_ptr = ThreadPool<task_t>::GetSingleInstance();

    int cnt = 10;
    while (cnt)
    {
        int rand_suffix = rand() % 4;
        tp_ptr->Enqueue(tasks[rand_suffix]);
        LOG(DEBUG, "%d\n", cnt--);
        //std::cout << cnt-- << std::endl;
        sleep(1);
    }

    tp_ptr->Stop();

    return 0;
}

// int main()
// {
//     Log lg;

//     //std::cout << lg.GetPrintLoc() << std::endl;

//     // lg.LoggingMessage(INFO, __FILE_NAME__, __LINE__, "this is a test data %d %s\n", 10, "what");
//     // lg.LoggingMessage(DEBUG, __FILE_NAME__, __LINE__, "this is a test data %d\n", 12);
//     // lg.LoggingMessage(ERROR, __FILE_NAME__, __LINE__, "this is a test data %d\n", 14);
//     // lg.LoggingMessage(FATAL, __FILE_NAME__, __LINE__, "this is a test data %d\n", 16);

//     LOG(INFO, "this is a test data %d %s\n", 10, "what");


//     Log lg2("FILE");
//     lg2.LoggingMessage(INFO, __FILE_NAME__, __LINE__, "this is a test data %d %s\n", 10, "what");
    
//     return 0;
// }

// int main()
// {
//     srand(time(nullptr) ^ getpid());
//     ThreadPool<task_t> thp(5);

//     thp.Init();

//     thp.Start();

//     int cnt = 10;
//     while (cnt)
//     {
//         int rand_suffix = rand() % 4;
//         thp.Enqueue(tasks[rand_suffix]);
//         std::cout << cnt-- << std::endl;
//         //sleep(1);
//     }

//     thp.Stop();

//     return 0;
// }

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