#include <iostream>
#include <string>

#include <unistd.h>
#include <pthread.h>

class ThreadData
{
public:
    ThreadData(std::string name = "thread []", int num = 0, int left = 10, int right = 10)
        :_name(name)
        ,_num(num)
        ,_left_oper(left)
        ,_right_oper(right)
    {}

    std::string getname()
    {
        return _name;
    }

    int getnum()
    {
        return _num;
    }

    int getleft()
    {
        return _left_oper;
    }

    int getright()
    {
        return _right_oper;
    }
private:
    std::string _name;
    int _num;

    int _left_oper;
    int _right_oper;
};

class ThreadReturn
{
public:
    ThreadReturn(int left = 0, int right = 0)
        :_left(left)
        ,_right(right)
    {}

    void add()
    {
        _result =  _left + _right;
    }

    std::string Print()
    {
        std::string print = std::to_string(_left) + " + " + std::to_string(_right) + " = " + std::to_string(_result);
        return print;
    }
private:
    int _left;
    int _right;
    int _result;
};


void* thread_run(void* args)
{
    ThreadData* td = static_cast<ThreadData*>(args);

    ThreadReturn* tr = new ThreadReturn(5,2);
    
    std::cout << "I am doing the task" << std::endl;

    tr->add();

    int cnt = 3;
    // while(cnt)
    // {
    //     std::cout << "I am the " << td->getname() << ", and my number is " << td->getnum() << ".";
    //     std::cout << " The rest time is " << cnt-- << ". "<< std::endl;
    //     sleep(1);
    // }
    delete td;
    //return reinterpret_cast<void*>(111);
    return tr;
}

int main()
{
    pthread_t tid;
    // 传入的参数不要定义在栈上，因为破坏了主线程的栈(线程一般要有自己独立的栈空间），所以一般定义在堆上
    ThreadData* td = new ThreadData("thread [1]", 1);
    // 传入的参数不仅可以是普通的参数，还可以传入类的对象指针
    int ret = pthread_create(&tid, nullptr, thread_run, td);

    if (ret != 0)
    {
        std::cout << "pthread_create error!" << std::endl;
        exit(-1);
    }

    // 返回值是一个二级指针，用于接收 thread_run 的返回值
    void* retval = nullptr;
    pthread_join(tid, &retval);

    ThreadReturn* retptr = static_cast<ThreadReturn*>(retval);
    //std::cout << "return value = " << (long long)retval << std::endl;
    std::cout << "return value: " << retptr->Print() << std::endl;
    delete retptr;
    return 0;
}

// void* func(void*)
// {   
//     while(true)
//     {
//         std::cout << "I am a thread, my pid = " << getpid() << "." << std::endl; 
//         sleep(1);
//     }
// }

// int main()
// {
//     pthread_t tid;
//     int ret = pthread_create(&tid, nullptr, func, nullptr);

//     std::cout << "ret =" << ret << ", tid = " << tid << std::endl;
//     while(true)
//     {
//         std::cout << "I am a main thread, my pid = " << getpid() << "." << std::endl; 
//         sleep(1);
//     }
//     return 0;
// }