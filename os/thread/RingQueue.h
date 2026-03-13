//
// Created by dsj on 2026/3/13.
//

#ifndef THREAD_RINGQUEUE_H
#define THREAD_RINGQUEUE_H
#include <semaphore.h>
#include <iostream>
#include <vector>
#include <pthread.h>



template <class T>
class RingQueue
{
private:
    void P(sem_t& sem)
    {
        sem_wait(&sem);
    }

    void V(sem_t& sem)
    {
        sem_post(&sem);
    }
public:
    RingQueue(int cap)
        :_cap(cap)
        ,_ring_queue(_cap)
        ,_produce_step(0)
        ,_consumer_step(0)
    {
        sem_init(&_space_sem, 0, _cap);
        sem_init(&_data_sem, 0, 0);
        pthread_mutex_init(&_mutex, nullptr);
    }

    void Enqueue(const T& task)
    {
        P(_space_sem);
        pthread_mutex_lock(&_mutex);
        _ring_queue[_produce_step++] = task;

        _data_sem %= _cap;
        pthread_mutex_unlock(&_mutex);
        V(_data_sem);
    }

    void Dequeue(T* task)
    {
        P(_data_sem);
        pthread_mutex_lock(&_mutex);

        *task = _ring_queue[_consumer_step++];
        _consumer_step %= _cap;

        pthread_mutex_unlock(&_mutex);
        V(_space_sem);
    }


private:
    std::vector<T> _ring_queue;
    int _cap;
    sem_t _space_sem;
    sem_t _data_sem;

    int _produce_step;
    int _consumer_step;

    pthread_mutex_t _mutex;
};

#endif //THREAD_RINGQUEUE_H
