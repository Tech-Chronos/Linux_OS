#include "BlockQueue.hpp"

const size_t capacity = 10;

void *Product(void *args)
{
    BlockQueue<int> *bq = static_cast<BlockQueue<int> *>(args);

    while (true)
    {
        int val = (rand() ^ getpid()) % 10 + 1; // 范围 1 ～ 11
        bq->Push(val);
        std::cout << "生产者产生的数据是：" << val << std::endl;
        sleep(1);
    }

    return nullptr;
}

void *Consume(void *args)
{
    BlockQueue<int> *bq = static_cast<BlockQueue<int> *>(args);

    //sleep(2);
    while (true)
    {
        int testnum = 0;
        bq->Pop(&testnum);

        std::cout << "消费者得到的数据是：" << testnum << std::endl;
    }
    return nullptr;
}

int main()
{
    srand(time(nullptr));

    BlockQueue<int> *bq = new BlockQueue<int>(capacity);
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