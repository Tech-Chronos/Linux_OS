#ifndef __TEST_THREAD__

#define __TEST_THREAD__
#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include <unistd.h>
#include <pthread.h>

using func_t = std::function<void(const std::string&)>;

// 由主线程来控制
class mythread
{
private:
    // 让线程开始执行任务，这里才是真正执行的地方
    void Excute()
    {
        std::cout << "start excute task!" << std::endl;
        _is_running = true;
        _func(_name);
        _is_running = false;
    }

    static void* ThreadTask(void* args)
    {
        mythread* self = static_cast<mythread*>(args);
        self->Excute();

        return nullptr;
    }
public:
    mythread(std::string name, func_t func)
        :_name(name)
        ,_func(func)
    {
        std::cout << _name << " thread has been constructed!" << std::endl;
    }


    void Start()
    {
        // 我要执行任务，必须有this指针来调用_func,所以参数需要传入this
        pthread_create(&_tid, nullptr, ThreadTask, this);
    }

    void Stop()
    {
        if (_is_running)
        {
            pthread_cancel(_tid);
            _is_running = false;
        }
        std::cout << _name << " has stopped!" << std::endl;
    }

    void Join()
    {
        pthread_join(_tid, nullptr);
        _is_running = false;
        std::cout << _name << " has joined!" << std::endl;
    }

private:   
    pthread_t _tid; // 线程id
    bool _is_running; // 线程是否正在执行任务

    std::string _name; // 线程的名称
    func_t _func;  // 执行的任务
};
#endif