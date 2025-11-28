#include "Semaphore_CP.hpp"
#include "Task.hpp"

void* Produce(void* args)
{
    CircularQueue<Task>* cq = static_cast<CircularQueue<Task>*>(args);

    while (true)
    {
        sleep(1);
        
        int left_val = rand() % 10 + 1;
        int right_val = rand() % 15 + 10;

        Task task(left_val, right_val);

        cq->Push(task);

        std::cout << "生产者放入数据：" << task.GetTask() << std::endl;
    }
}

void* Consume(void* args)
{
    CircularQueue<Task>* cq = static_cast<CircularQueue<Task>*>(args);

    sleep(2);
    while (true)
    {
        Task task;
        
        cq->Pop(&task);

        task();
        std::cout << "消费者拿出数据：" << task.DealResult() << std::endl;
    }
}

int main()
{
    srand(time(nullptr) ^ getpid());
    pthread_t producer1, consumer1;
    pthread_t producer2, consumer2;
    //pthread_t producer3, consumer3;

    CircularQueue<Task>* cq = new CircularQueue<Task>(10);
    pthread_create(&producer1, nullptr, Produce, cq);
    pthread_create(&producer2, nullptr, Produce, cq);
    //pthread_create(&producer3, nullptr, Produce, cq);
    pthread_create(&consumer1, nullptr, Consume, cq);
    pthread_create(&consumer2, nullptr, Consume, cq);
    //pthread_create(&consumer3, nullptr, Consume, cq);


    pthread_join(producer1, nullptr);
    pthread_join(producer2, nullptr);
    //pthread_join(producer3, nullptr);
    pthread_join(consumer1, nullptr);
    pthread_join(consumer2, nullptr);
    //pthread_join(consumer3, nullptr);
    return 0;
}

// 生产者
// void* Produce(void* args)
// {
//     CircularQueue<int>* cq = static_cast<CircularQueue<int>*>(args);

//     while (true)
//     {
//         sleep(1);
//         int val = rand() % 10 + 1;

//         cq->Push(val);

//         std::cout << "生产者放入数据：" << val << std::endl;
//     }
// }

// void* Consume(void* args)
// {
//     CircularQueue<int>* cq = static_cast<CircularQueue<int>*>(args);

//     sleep(2);
//     while (true)
//     {
//         int val = 0;
        
//         cq->Pop(&val);

//         std::cout << "消费者拿出数据：" << val << std::endl;
//     }
// }

// int main()
// {
//     srand(time(nullptr) ^ getpid());
//     pthread_t producer1, consumer1;
//     pthread_t producer2, consumer2;
//     pthread_t producer3, consumer3;

//     CircularQueue<int>* cq = new CircularQueue<int>(10);
//     pthread_create(&producer1, nullptr, Produce, cq);
//     pthread_create(&producer2, nullptr, Produce, cq);
//     pthread_create(&producer3, nullptr, Produce, cq);
//     pthread_create(&consumer1, nullptr, Consume, cq);
//     pthread_create(&consumer2, nullptr, Consume, cq);
//     pthread_create(&consumer3, nullptr, Consume, cq);


//     pthread_join(producer1, nullptr);
//     pthread_join(producer2, nullptr);
//     pthread_join(producer3, nullptr);
//     pthread_join(consumer1, nullptr);
//     pthread_join(consumer2, nullptr);
//     pthread_join(consumer3, nullptr);
//     return 0;
// }