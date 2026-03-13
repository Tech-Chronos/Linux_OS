//
// Created by dsj on 2026/3/13.
//

#ifndef THREAD_THREAD_H
#define THREAD_THREAD_H
#include <iostream>
#include <string>
#include <functional>
#include <pthread.h>

template <typename T>
class Thread
{
private:
    void Excute(std::string& name)
    {
        _func(name);
    }

    static void* thread_func(void* args)
    {
        auto* thread = static_cast<Thread*>(args);
        thread->Excute(thread->_name);

        return nullptr;
    }
public:
    Thread(const std::string& name, const T& func)
        :_name(name)
        ,_func(func)
        ,_isrunning(false)
    {
    }

    bool Create()
    {
        _isrunning = true;
        pthread_create(&_tid, nullptr, thread_func, this);
    }

    void Join()
    {
        pthread_join(_tid, nullptr);
        _isrunning = false;
    }

    void Cancel()
    {
        if (_isrunning)
        {
            pthread_cancel(_tid);
            _isrunning = false;
        }
    }

    void Detach()
    {
        pthread_detach(_tid);
    }

private:
    bool _isrunning;
    pthread_t _tid; // 线程tid
    std::string _name; // 线程名字
    T _func; // 线程执行的函数
};

#endif //THREAD_THREAD_H
