#include "BlockQueue.hpp"
#include "Task.hpp"

const size_t capacity = 10;

void *Product(void *args)
{
    BlockQueue<task_t> *bq = static_cast<BlockQueue<task_t> *>(args);

    while (true)
    {
        int suffix = rand() % (sizeof(tasks) / sizeof(tasks[1])); // 范围 1～ 11

        // 构造任务对象
        task_t task = tasks[suffix];

        bq->Push(task);
        std::cout << "生产者产生了任务！" << std::endl;
        sleep(1);
    }

    return nullptr;
}

void *Consume(void *args)
{
    BlockQueue<task_t> *bq = static_cast<BlockQueue<task_t> *>(args);

    sleep(2);
    while (true)
    {
        task_t task;
        bq->Pop(&task);

        task();
        //std::cout << "消费者处理完的任务是：" << t.DealResult() << std::endl;
    }
    return nullptr;
}

int main()
{
    srand(time(nullptr));

    BlockQueue<task_t> *bq = new BlockQueue<task_t>(capacity);
    pthread_t producer;
    pthread_t consumer;

    // 创建线程的时候需要把资源传递过去

    // 创建生产者线程
    pthread_create(&producer, nullptr, Product, bq);

    // 创建消费者线程
    pthread_create(&consumer, nullptr, Consume, bq);

    // 回收进程
    pthread_join(producer, nullptr);

    pthread_join(consumer, nullptr);
    return 0;
}

// void *Product(void *args)
// {
//     BlockQueue<Task> *bq = static_cast<BlockQueue<Task> *>(args);

//     while (true)
//     {
//         int left_val = (rand() ^ getpid()) % 10 + 1; // 范围 1～ 11
//         int right_val = (rand() ^ getpid()) % 20 + 11; // 范围 11～ 30

//         // 构造任务对象
//         Task t(left_val, right_val);

//         bq->Push(t);
//         std::cout << "生产者产生的任务是：" << t.GetTask() << std::endl;
//         sleep(1);
//     }

//     return nullptr;
// }

// void *Consume(void *args)
// {
//     BlockQueue<Task> *bq = static_cast<BlockQueue<Task> *>(args);

//     sleep(2);
//     while (true)
//     {
//         Task t;
//         bq->Pop(&t);

//         t();
//         std::cout << "消费者处理完的任务是：" << t.DealResult() << std::endl;
//     }
//     return nullptr;
// }

// int main()
// {
//     srand(time(nullptr));

//     BlockQueue<Task> *bq = new BlockQueue<Task>(capacity);
//     pthread_t producer;
//     pthread_t consumer;

//     // 创建线程的时候需要把资源传递过去

//     // 创建生产者线程
//     pthread_create(&producer, nullptr, Product, bq);

//     // 创建消费者线程
//     pthread_create(&consumer, nullptr, Consume, bq);

//     // 回收进程
//     pthread_join(producer, nullptr);

//     pthread_join(consumer, nullptr);
//     return 0;
// }

// const size_t capacity = 10;

// void *Product(void *args)
// {
//     BlockQueue<int> *bq = static_cast<BlockQueue<int> *>(args);

//     while (true)
//     {
//         int val = (rand() ^ getpid()) % 10 + 1; // 范围 1 ～ 11
//         bq->Push(val);
//         std::cout << "生产者产生的数据是：" << val << std::endl;
//         sleep(1);
//     }

//     return nullptr;
// }

// void *Consume(void *args)
// {
//     BlockQueue<int> *bq = static_cast<BlockQueue<int> *>(args);

//     //sleep(2);
//     while (true)
//     {
//         int testnum = 0;
//         bq->Pop(&testnum);

//         std::cout << "消费者得到的数据是：" << testnum << std::endl;
//     }
//     return nullptr;
// }

// int main()
// {
//     srand(time(nullptr));

//     BlockQueue<int> *bq = new BlockQueue<int>(capacity);
//     pthread_t producer;
//     pthread_t consumer;

//     // 创建线程的时候需要把资源传递过去

//     // 创建生产者线程
//     pthread_create(&producer, nullptr, Product, bq);

//     // 创建消费者线程
//     pthread_create(&consumer, nullptr, Consume, bq);

//     // 回收进程
//     pthread_join(producer, nullptr);

//     pthread_join(consumer, nullptr);
//     return 0;
// }