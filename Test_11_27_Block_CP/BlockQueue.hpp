#ifndef __BLOCK_QUEUE_HPP__

#define __BLOCK_QUEUE_HPP__

#include <iostream>
#include <queue>

#include <unistd.h>
#include <pthread.h>

// 实现基于阻塞队列的生产消费模型

template <class T>
class BlockQueue
{
private:
    bool IsFull()
    {
        return _block_queue.size() == _max_capacity;
    }

    bool IsEmpty()
    {
        return _block_queue.size() == 0;
    }
public:
    BlockQueue(size_t max_capacity)
        :_max_capacity(max_capacity)
    {
        // 对互斥锁和条件变量初始化
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_cond_p, nullptr);
        pthread_cond_init(&_cond_c, nullptr);
    }

    void Push(const T& val)
    {
        pthread_mutex_lock(&_mutex);
        // 当阻塞队列满的时候，就让生产者不要插入，让他在条件变量上等着
        while (IsFull())
        {
            // 要注意在等待条件变量的时候这个线程会释放锁！
            pthread_cond_wait(&_cond_p, &_mutex);
        }
        // 到这里要么是两种情况： 
        // 1. 队列本来就不满
        // 2. 线程被唤醒了
        _block_queue.push(val);

        // 到这里了，说明生产者一定向阻塞队列中插入数据了，他知道队列肯定不为空！所以可以通知消费者来消费了
        // 这里可以放在解锁之前或者之后，如果放在之前，就是唤醒消费者进程，但是我还没释放锁：
        // 如果抢到锁了就继续执行，如果没抢到，就阻塞在锁哪里，而不是在条件变量那里阻塞；
        // 但是可能消费者没有阻塞，也会通知，可以看看比率：如果数量大于75%，就进行通知，否则就不通知
        //if ((_block_queue.size() * 100) / _max_capacity > 75)
            pthread_cond_signal(&_cond_c);

        pthread_mutex_unlock(&_mutex);

    }

    void Pop(T* val)
    {
        pthread_mutex_lock(&_mutex);
        // 当阻塞队列为空的时候，不让消费者消费，让他在条件变量上等着
        while (IsEmpty())
        {
            pthread_cond_wait(&_cond_c, &_mutex);
        }
        // 到这里要么是两种情况： 
        // 1. 队列本来就不空
        // 2. 线程被唤醒了
        *val = _block_queue.front();

        _block_queue.pop();

        // 这里也采用比率，如果小于35%就进行通知
        //if ((_block_queue.size() * 100) / _max_capacity < 35)
            pthread_cond_signal(&_cond_p);

        pthread_mutex_unlock(&_mutex);
    }

    ~BlockQueue()
    {
        // 对互斥锁和条件变量释放
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond_p);
        pthread_cond_destroy(&_cond_c);
    }
private:
    std::queue<T> _block_queue;
    pthread_mutex_t _mutex; // 一把互斥锁
    pthread_cond_t _cond_p; // 生产者的条件变量
    pthread_cond_t _cond_c; // 消费者的条件变量

    size_t _max_capacity;
};


#endif