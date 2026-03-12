//
// Created by dsj on 2026/3/9.
//
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/shm.h>

using namespace std;

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <signal.h>

void PrintSig(sigset_t &pending)
{
    std::cout << "Pending bitmap:";
    for(int signo = 31; signo > 0; signo--)
    {
        if(sigismember(&pending, signo))//检测pending位图中的信号是否存在
        {
            std::cout << "1";
        }
        else
        {
            std::cout << "0";
        }
    }
    std::cout << std::endl;
}

int main()
{
    //1.屏蔽2号信号
    sigset_t block, oblock;//栈上定义的变量可能为随机值
    sigemptyset(&block);// 清空信号集
    sigemptyset(&oblock);
    sigaddset(&block, 2);// 此时并未将2号信号写入到pcb block位图中，而是在栈中

    // 开始屏蔽2号信号，即设置进入内核中
    int x = sigprocmask(SIG_SETMASK, &block, &oblock);
    (void)x;// 取消无返回值接收报警
    std::cout << "block 2 signal sucess" << std::endl;

    int count = 0;
    while(true)
    {
        ++count;
        //2.获取进程的pending位图
        sigset_t pending;
        sigemptyset(&pending);
        x = sigpending(&pending);
        assert(x == 0);

        //3.打印pending位图中收到信号
        PrintSig(pending);

        if (count == 20)
        {
            sigprocmask(SIG_UNBLOCK, &block, &oblock);
        }
        sleep(1);
    }

    return 0;
}
