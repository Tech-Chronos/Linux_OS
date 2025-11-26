#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include <unistd.h>
#include <pthread.h>

// c++11 多线程库
void threadrun(std::string name)
{
    while (1)
    {
        std::cout << "hello, my name is " << name << std::endl;
        sleep(1);
    }
}

int main()
{
    std::string name = "thread [1]";
    std::thread thread(threadrun, std::move(name));

    thread.join();
    return 0;
}



// 创建多个线程，暂时未同步or互斥
// int num = 10;

// char* PrintHex(int printnum)
// {
//     char* buffer = new char[128];

//     snprintf(buffer, 128, "0x%x", printnum);

//     return buffer;
// }

// void* threadrun(void* args)
// {
//     // 线程的分离,分离之后无需等待
//     //pthread_detach(pthread_self());

//     const char* name = static_cast<const char*>(args);
//     while (true)
//     {
//         sleep(1);

//         // pthread_exit 直接把返回值带出去
//         //pthread_exit((void*)name);
//         std::cout << name << std::endl;
//         break;
//     }

//     //return nullptr;

//     // exit有一个线程退出整个进程就会退出
//     //exit(-1);

//     // 只退出线程
    
//     return (void*)name;
// }

// int main()
// {
//     std::vector<pthread_t> vec_p;

//     for (int i = num; i > 0; --i)
//     {
//         pthread_t tid;

//         char* buffer = new char[128];

//         // 这个是非⚛️操作
//         snprintf(buffer, 128, "thread [%d]", i);

//         pthread_create(&tid, nullptr, threadrun, buffer);

//         // 将tid放到vector中
//         vec_p.emplace_back(tid);
//     }

//     for (auto tid : vec_p)
//     {
//         // 主线程可以调用 cancel, 表示退出对应的新线程
//         // 退出的值是 (void*)-1
//         //pthread_cancel(tid);

//         // 也可以在main线程detach
//         pthread_detach(tid);

//         void* retval;
//         int ret = pthread_join(tid, &retval);

//         std::cout << "The return value of join : " << ret << std::endl;
//         //std::cout << "retval : " << (long long)retval << std::endl;

//         char* retv = PrintHex((long long)retval);
//         std::cout << "retval = " << retv << std::endl;

//         delete retv;
//         //char* name = static_cast<char*>(retval);

//         //std::cout << "name : "<< name << std::endl;
//     }

//     //sleep(100);
//     return 0;
// }



// 创建单个线程
// class ThreadData
// {
// public:
//     ThreadData(std::string name = "thread [0]", int left = 0, int right = 0, int result = 0)
//         :_result(result)
//         ,_name(name)
//         ,_left(left)
//         ,_right(right)
//     {}

//     std::string getname()
//     {
//         return _name;
//     }

//     int getleft()
//     {
//         return _left;
//     }

//     int getright()
//     {
//         return _right;
//     }

//     int getresult()
//     {
//         return _result;
//     }

//     void Add()
//     {
//         _result = _left + _right;
//     }

// private:
//     std::string _name;

//     int _left;
//     int _right;
//     int _result;
// };

// void* thread_run(void* args)
// {
//     ThreadData* ptd = static_cast<ThreadData*>(args);

//     ptd->Add();

//     std::cout << "new thread starts doing task!" << std::endl;
//     while (true)
//     {
//         std::cout << ptd->getname() << ": " << ptd->getleft() << \
//          " + " << ptd->getright() << " = " << ptd->getresult() << std::endl;

//         sleep(1);

//         break;
//     }

//     return ptd;
// }

// int main()
// {
//     pthread_t tid;

//     ThreadData* ptd = new ThreadData("thread [1]", 10, 20);
//     pthread_create(&tid, nullptr, thread_run, ptd);

//     std::cout << "main thread is waiting for new thread!" << std::endl;
//     void* retval = nullptr;
//     pthread_join(tid, &retval);

//     ThreadData* ret = static_cast<ThreadData*>(retval);

//     std::cout << "main thread joins successfully, result = " << ret->getresult() << std::endl;
//     return 0;
// }