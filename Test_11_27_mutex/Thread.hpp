#ifndef __THREA__TEST__
#define __THREA__TEST__
#include "LockGuard.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include <unistd.h>
#include <pthread.h>

class threaddata
{
public:
    threaddata(pthread_mutex_t* mutex, std::string& name)
        :_mutex(mutex)
        ,_name(name)
    {}

    std::string getname() const
    {
        return _name;
    }

    pthread_mutex_t* getmutex() const
    {
        return _mutex;
    }

private:
    pthread_mutex_t* _mutex;
    std::string _name;
};

using func_t = std::function<void(const threaddata& _td)>;

class mythread
{
private:
    void Excute()
    {
        _is_running = true;
        std::cout << _td.getname() << " is excuting the task!" << std::endl;
        _func(_td);
        _is_running = false;
    }

    static void* ThreadTask(void* args)
    {
        mythread* self = static_cast<mythread*>(args);
        self->Excute();

        return nullptr;
    }
public:
    mythread(threaddata td, func_t func)
        :_td(td)
        ,_func(func)
    {
        std::cout << _td.getname() << " has been constructed!" << std::endl;
    }

    void Start()
    {
        pthread_create(&_tid, nullptr, ThreadTask, this);
    }

    void Cancel()
    {
        if (_is_running)
        {
            pthread_cancel(_tid);
            _is_running = false;
            std::cout << _td.getname() << " has been cancelled!" << std::endl;
        }
    }

    void Join()
    {
        pthread_join(_tid, nullptr);
        pthread_mutex_destroy(_td.getmutex());
        std::cout << _td.getname() << " has been joined!" << std::endl;
    }

private:
    pthread_t _tid;
    threaddata _td;

    bool _is_running;
    func_t _func;
};

#endif