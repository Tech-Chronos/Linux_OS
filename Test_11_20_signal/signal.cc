#include <iostream>

#include <unistd.h>
#include <signal.h>

void PrintPending(sigset_t pending_set)
{
    std::cout << "my pid is [" << getpid() << "], pending table is ->";
    for (int i = 31; i > 0; --i)
    {
        // 判断 i 号信号是否在 pending 位图中
        if (sigismember(&pending_set, i))
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

// 之后获取到信号立即处理
void handler(int signo)
{
    std::cout << "get signo = " << signo << std::endl;

    sigset_t pendingset;
    sigemptyset(&pendingset);
    sigpending(&pendingset);

    std::cout << "--------------------------------------" << std::endl;

    // 是递达之前pending表就置0了
    PrintPending(pendingset);

    std::cout << "--------------------------------------" << std::endl;

}

// 信号的保存
int main()
{
    // 0. 2号信号自定义为handler
    signal(2, handler);
    // 1. 定义两个位图
    sigset_t block_set, old_set;

    // 2. 清空位图
    sigemptyset(&block_set);
    sigemptyset(&old_set);

    // 3. 设置信号（2信号被阻塞）
    sigaddset(&block_set, 2);

    // 4. 发送给进程
    sigprocmask(SIG_BLOCK, &block_set, &old_set);

    sigset_t pending_set;
    sigemptyset(&pending_set);

    // pending表中会保存当前收到的信号，如果被阻塞的就不会处理
    int cnt = 5;
    while (true)
    {
        // 5. 打印pending
        sigpending(&pending_set);
        PrintPending(pending_set);
        sleep(1);

        // 5s之后唤醒2号信号
        --cnt;
        if (cnt == 0)
        {
            std::cout << "唤醒 2 号信号 ！" << std::endl;
            sigprocmask(SIG_UNBLOCK, &block_set, nullptr);
        }
    }

    return 0;
}