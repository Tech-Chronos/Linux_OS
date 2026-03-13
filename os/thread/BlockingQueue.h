//
// Created by dsj on 2026/3/13.
//

#ifndef THREAD_BLOCKINGQUEUE_H
#define THREAD_BLOCKINGQUEUE_H
#include <iostream>
#include <queue>
#include <pthread.h>

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
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_cond_c, nullptr);
        pthread_cond_init(&_cond_p, nullptr);
    }

    void Push(const T& task)
    {
        pthread_mutex_lock(&_mutex);
        // 没空间，等待消费者消费
        while (IsFull())
        {
            // 等待消费者给我发信号
            pthread_cond_wait(&_cond_p, &_mutex);
        }

        _block_queue.push(task);

        pthread_cond_signal(&_cond_c);
        pthread_mutex_unlock(&_mutex);

    }

    void Pop(T* task)
    {
        pthread_mutex_lock(&_mutex);
        while (IsEmpty())
        {
            pthread_cond_wait(&_cond_c, &_mutex);
        }

        *task = _block_queue.front();
        _block_queue.pop();
        pthread_cond_signal(&_cond_p);
        pthread_mutex_unlock(&_mutex);

    }

    ~BlockQueue()
    {
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond_c);
        pthread_cond_destroy(&_cond_p);
    }
private:
    std::queue<T> _block_queue;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond_c;
    pthread_cond_t _cond_p;

    size_t _max_capacity;
};

#endif //THREAD_BLOCKINGQUEUE_H
