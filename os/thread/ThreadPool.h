//
// Created by dsj on 2026/3/13.
//

#ifndef THREAD_THREADPOOL_H
#define THREAD_THREADPOOL_H
#include "Thread.h"
#include <vector>

using func_t = std::function<void(std::string&)>;

pthread_mutex_t singlton_mutex = PTHREAD_MUTEX_INITIALIZER;

template <typename T>
class ThreadPool
{
private:
    bool IsEmpty()
    {
        return _tasks.empty();
    }

    void Lock()
    {
        pthread_mutex_lock(&_mutex);
    }

    void UnLock()
    {
        pthread_mutex_unlock(&_mutex);
    }

    void Wait()
    {
        pthread_cond_wait(&_cond, &_mutex);
    }

    void Signal()
    {
        pthread_cond_signal(&_cond);
    }

    void BroadCast()
    {
        pthread_cond_broadcast(&_cond);
    }

    void HandlerTask(std::string& name)
    {
        while (true)
        {
            Lock();

            // 什么时候等待
            // 在运行并且没任务，等待
            while (IsEmpty() && _isrunning)
            {
                ++_sleep_threads_num;
                Wait();
                --_sleep_threads_num;
            }

            /// 什么时候退出
            // 没任务并且不再运行，退出
            if (IsEmpty() && !_isrunning)
            {
                UnLock();
                break;
            }
            T task = _tasks.front();
            _tasks.pop();

            UnLock();
            task(name);
        }
    }

    ThreadPool(int thread_num)
            :_isrunning(false)
            ,_thread_num(thread_num)
            ,_sleep_threads_num(0)
    {
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_cond, nullptr);
    }
public:


    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    static ThreadPool* ThreadPoolSinglton(int thread_num = 5)
    {
        if (_thread_instance == nullptr)
        {
            pthread_mutex_lock(&singlton_mutex);
            if (_thread_instance == nullptr)
            {
                _thread_instance = new ThreadPool<T>(thread_num);

                _thread_instance->Init();
                _thread_instance->Start();
            }
            pthread_mutex_unlock(&singlton_mutex);
        }
        return _thread_instance;
    }

    void Init()
    {
        _isrunning = true;
        for (int i = 0; i < _thread_num; ++i)
        {
            std::string name = "Thread[" + std::to_string(i) + "]";
            func_t task = std::bind(&ThreadPool::HandlerTask, this, std::placeholders::_1);
            _threads.emplace_back(name, task);
        }
    }

    void Start()
    {
        for (int i = 0; i < _threads.size(); ++i)
        {
            _threads[i].Create();
        }
    }

    void Enqueue(const T& task)
    {
        Lock();
        if (_isrunning)
        {
            _tasks.push(task);
            Signal();
        }
        UnLock();
    }

    void Stop()
    {
        Lock();
        _isrunning = false;
        // 唤醒所有的线程，防止有线程在等待任务
        BroadCast();

        UnLock();

        // 锁给消费者线程抢，这边阻塞回收
        for (auto& thread : _threads)
        {
            thread.Join();
        }
    }

    ~ThreadPool()
    {
        _isrunning = false;
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
    }

private:
    bool _isrunning;
    std::vector<Thread<T>> _threads;
    std::queue<T> _tasks;

    int _thread_num;

    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

    int _sleep_threads_num;

    static ThreadPool* _thread_instance;
};

template<class T>
ThreadPool<T>* ThreadPool<T>::_thread_instance = nullptr;

#endif //THREAD_THREADPOOL_H
