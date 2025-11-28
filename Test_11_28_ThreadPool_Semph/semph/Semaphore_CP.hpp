#pragma once

#include <iostream>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


// 对于多生产多消费的模型，信号量的申请本身虽然是原子的
// 但是下标suffix的访问并不是原子的，会出错，所以要加锁！
// 要加两把锁，因为生产者和消费者的信号量并不一样，所以访问的也不一样！
template<class T>
class CircularQueue
{
private:
    // 信号量本身就是判断条件，如果申请成功就会继续执行
    // 申请失败就会阻塞
    void P(sem_t& sem)
    {
        sem_wait(&sem);
    }

    void V(sem_t& sem)
    {
        sem_post(&sem);
    }

    void Lock(pthread_mutex_t& mutex)
    {
        pthread_mutex_lock(&mutex);
    }

    void Unlock(pthread_mutex_t& mutex)
    {
        pthread_mutex_unlock(&mutex);
    }

public:
    CircularQueue(size_t max_capacity)
        :_max_capacity(max_capacity)
        ,_circular_queue(max_capacity)
        ,_p_suffix(0)
        ,_c_suffix(0)
    {
        sem_init(&_sem_space, 0, max_capacity);
        sem_init(&_sem_data, 0, 0);

        pthread_mutex_init(&_p_mutex, nullptr);
        pthread_mutex_init(&_c_mutex, nullptr);
    }

    void Push(const T& val)
    {
        // 生产者申请空间信号量
        // 申请成功就插入
        // 申请失败就阻塞在信号量这里
        // 如果是多生产多消费，那么
        P(_sem_space);
        // 因为申请信号量本身是原子的，所以本身不用进行加锁，可以申请完信号量之后
        // 在进行加锁。
        Lock(_p_mutex);
        _circular_queue[_p_suffix] = val;
        ++_p_suffix;
        _p_suffix %= _max_capacity;

        Unlock(_p_mutex);
        // 插入数据成功后，对于生产者来说可以归还一个消费者的数据信号量
        V(_sem_data);
    }

    void Pop(T* val)
    {
        P(_sem_data);
        Lock(_c_mutex);
        *val = _circular_queue[_c_suffix];
        ++_c_suffix;
        _c_suffix %= _max_capacity;

        Unlock(_c_mutex);
        // 消费者拿走一个数据，可以归还一个空间信号量
        V(_sem_space);
    }

    ~CircularQueue()
    {
        sem_destroy(&_sem_data);
        sem_destroy(&_sem_space);

        pthread_mutex_destroy(&_c_mutex);
        pthread_mutex_destroy(&_p_mutex);
    }

private:
    size_t _max_capacity;
    std::vector<T> _circular_queue; // 用vector模拟循环队列
    sem_t _sem_space; // 对于生产者来说的空间信号量
    sem_t _sem_data; // 对于消费者来说的数据信号量

    size_t _p_suffix; // 对于生产者的插入下标
    size_t _c_suffix; // 对于消费者的获取下标

    pthread_mutex_t _p_mutex; // 对于生产者的锁
    pthread_mutex_t _c_mutex; // 对于消费者的锁
};



// 对于单生产单消费的信号量
// template<class T>
// class CircularQueue
// {
// private:
//     bool P(sem_t& sem)
//     {
//         sem_wait(&sem);
//         return true;
//     }

//     bool V(sem_t& sem)
//     {
//         sem_post(&sem);
//         return true;
//     }
// public:
//     CircularQueue(size_t max_capacity)
//         :_max_capacity(max_capacity)
//         ,_circular_queue(max_capacity)
//         ,_p_suffix(0)
//         ,_c_suffix(0)
//     {
//         sem_init(&_sem_space, 0, max_capacity);
//         sem_init(&_sem_data, 0, 0);
//     }

//     void Push(const T& val)
//     {
//         // 生产者申请空间信号量
//         // 申请成功就插入
//         // 申请失败就阻塞在信号量这里
//         if (P(_sem_space))
//         {
//             _circular_queue[_p_suffix] = val;
//             ++_p_suffix;
//             _p_suffix %= _max_capacity;

//             // 插入数据成功后，对于生产者来说可以归还一个消费者的数据信号量
//             V(_sem_data);
//         }

//     }

//     void Pop(T* val)
//     {
//         if (P(_sem_data))
//         {
//             *val = _circular_queue[_c_suffix];

//             ++_c_suffix;
//             _c_suffix %= _max_capacity;

//             // 消费者拿走一个数据，可以归还一个空间信号量
//             V(_sem_space);
//         }
//     }

//     ~CircularQueue()
//     {
//         sem_destroy(&_sem_data);
//         sem_destroy(&_sem_space);
//     }

// private:
//     size_t _max_capacity;
//     std::vector<T> _circular_queue; // 用vector模拟循环队列
//     sem_t _sem_space; // 对于生产者来说的空间信号量
//     sem_t _sem_data; // 对于消费者来说的数据信号量

//     size_t _p_suffix; // 对于生产者的插入下标
//     size_t _c_suffix; // 对于消费者的获取下标
// };