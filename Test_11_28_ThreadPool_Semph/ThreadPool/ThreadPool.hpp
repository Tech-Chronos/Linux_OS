#pragma once
#include "Thread.hpp"
#include <iostream>
#include <vector>
#include <queue>

// void test()
// {
//     std::cout << "test" << std::endl;
//     sleep(1);
// }

template <class T>
class ThreadPool
{
private:
    void Lock()
    {
        pthread_mutex_lock(&_mutex);
    }

    void Unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }

    bool IsEmpty()
    {
        return _tasks.size() == 0;
    }

    void wait()
    {
        pthread_cond_wait(&_cond, &_mutex);
    }

    void WakeUp()
    {
        pthread_cond_signal(&_cond);
    }

    void WakeUpAll()
    {
        pthread_cond_broadcast(&_cond);
    }

    void HandlerTask(std::string &name)
    {
        while (true)
        {
            Lock();
            // 1. 空，线程运行 2. 空，线程不运行  4. 非空，线程不运行
            while (IsEmpty() && _is_running)
            {
                // 条件变量进行等待条件
                wait();
            }

            //
            if (IsEmpty() && !_is_running)
            {
                Unlock();
                break;
            }

            // 出循环表示通知线程有任务了
            task_t task = _tasks.front();

            _tasks.pop();
            Unlock();

            // 释放锁之后执行任务，可并发执行
            task(name);
        }
    }

public:
    ThreadPool(int thread_num)
        : _thread_num(thread_num), _is_running(false)
    {
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_cond, nullptr);
    }

    void Init()
    {
        std::function<void(std::string & name)> func = std::bind(&ThreadPool::HandlerTask, this, std::placeholders::_1);
        for (int i = 0; i < _thread_num; ++i)
        {
            std::string name = "thread [" + std::to_string(i + 1) + "]";
            _threads.emplace_back(name, func);
        }
    }

    void Start()
    {
        _is_running = true;
        for (auto &thread : _threads)
        {
            thread.Start();
        }
    }

    // 任务入队列
    void Enqueue(const task_t &task)
    {
        Lock();
        if (!_is_running)
        {
            Unlock();
            return; // 或 throw
        }
        _tasks.push(task);
        WakeUp();
        Unlock();
    }

    void Stop()
    {
        Lock();
        _is_running = false;
        WakeUpAll();

        Unlock();

        for (auto &thread : _threads)
        {
            thread.Join();
        }
    }

    ~ThreadPool()
    {
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
    }

private:
    int _thread_num; // 有几个线程
    bool _is_running;
    std::vector<mythread> _threads; // 用vector管理线程

    std::queue<task_t> _tasks; // 任务队列

    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
};