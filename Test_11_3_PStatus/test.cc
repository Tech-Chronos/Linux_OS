#include <iostream>
#include <unistd.h>
#include <sys/types.h>
using namespace std;

void testS_status()
{
    // 屏幕输出大约是 几KB/s 或更低；
    // 而 CPU 写内存的速度是 几GB/s。
    // 所以更多的时间在终端的等待队列，只有少部分时间在cpu的等待队列
    while (1)
    {
        cout << "hello" << endl;
    }
}


// 进程暂停状态，例如debug
void testT_status()
{
    while (1)
    {
        cout << "T status" << endl;
    }
}

// 僵尸进程
void testZb_status()
{
    pid_t id = fork();
    if (id == 0)
    {
        // child
        const int n = 5;
        for (int i = 0; i < n; ++i)
        {
            cout << "pid: " << getpid() << endl;
            sleep(1);
        }
    }
    else
    {
        // parent
        while (1)
        {
            cout << "pid: " << getpid() << endl;
            sleep(1);
        }
    }
}


// 孤儿进程
void testOp_status()
{
    pid_t id = fork();
    if (id == 0)
    {
        while (1)
        {
            cout << "child process: id = " << getpid() << endl;
            sleep(1);
        }
    }
    else
    {
        for (int i = 0; i < 5; ++i)
        {
            cout << "parent process: id = " << getpid() << endl;
            sleep(1);
        }
    }
}

int main()
{
    //testT_status();
    testOp_status();
    return 0;
}