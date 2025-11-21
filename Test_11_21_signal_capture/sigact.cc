#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void notice(int signo)
{

    std::cout << "***************************************************************" << std::endl;
    std::cout << "father process has received signo [" << signo << "], child process has quited!" << std::endl;

    std::cout << "my pid is " << getpid() << std::endl;

    while(true)
    {
        // 非阻塞等待，因为可能有的子进程退出，有的不退出
        int ret = waitpid(-1, nullptr, WNOHANG);
        if (ret > 0)
        {
            std::cout << "father process wait success!" << std::endl;
        }
        else if (ret < 0)
        {
            std::cout << "children have all quited!" << std::endl;
            break;
        }
        else
        {
            continue;
        }
    }
    
    std::cout << "***************************************************************" << std::endl;
}

void DoOtherThing()
{
    std::cout << "I am father process, I am doing other things!" << std::endl;
    sleep(1);
}

int main()
{
    //signal(SIGCHLD, notice);

    //可以手动设置默认，和自动的默认不一样！
    signal(SIGCHLD, SIG_IGN);
    pid_t id = 0;

    for (int i = 2; i > 0; --i)
    {
        id = fork();
        if (id == 0)
            break;
    }

    // 子进程退出会向父进程发信号 SIGCHLD
    if (id == 0)
    {
        int cnt = 5;
        while (cnt--)
        {
            std::cout << "I am the child process, my pid is " << getpid() << std::endl;
            sleep(1);
        }

        std::cout << "child process quit" << std::endl;
        exit(0);
    }
    else
    {
        alarm(15);
        while (true)
        {
            DoOtherThing();
        }
    }

    return 0;
}

// 关于volatile的用法,必须从内存中读数据
// volatile int flag = 0;

// void handler(int signo)
// {

//     std::cout << "I have received the [" << signo << "] signal !" << std::endl;

//     flag = 1;
// }

// int main()
// {
//     signal(2, handler);

//     while (!flag);

//     std::cout << "process quit normally !" << std::endl;
// }

// void PrintPending(sigset_t pending_set)
// {
//     for (int i = 31; i > 0; --i)
//     {
//         if (sigismember(&pending_set, i))
//         {
//             std::cout << "1";
//         }
//         else
//         {
//             std::cout << "0";
//         }
//     }
//     std::cout << std::endl;
// }

// void handler(int signo)
// {
//     while (true)
//     {
//         // 收到信号的时候一直在这个循环中，检测是否会阻塞正在递达的信号
//         std::cout << "I have receiverd the [" << signo << "] signal !" << std::endl;

//         // 直接打印看看是否会阻塞(结果就是会阻塞的)
//         sigset_t pending_set;
//         sigpending(&pending_set);
//         PrintPending(pending_set);
//         sleep(1);
//     }

// }

// int main()
// {
//     struct sigaction action, oldaction;

//     action.sa_flags = 0;

//     // 由于信号在递达时会自动阻塞当前的信号（OS自己做的），然后 sa_mask 也可以手动添加你想要阻塞的信号
//     // 这里全部添加一下，因为有些信号是不能被阻塞的，必须被调用
//     sigemptyset(&action.sa_mask);

//     for (int i = 0; i < 32; ++i)
//     {
//         sigaddset(&action.sa_mask, i);
//     }

//     action.sa_handler = handler;

//     sigaction(2, &action, &oldaction);

//     while (true)
//     {
//         sigset_t pending_set;
//         sigpending(&pending_set);
//         PrintPending(pending_set);
//         sleep(1);
//     }

//     return 0;
// }

// int main()
// {
//     struct sigaction action, oldaction;

//     action.sa_flags = 0;
//     sigemptyset(&action.sa_mask);

//     action.sa_handler = handler;

//     sigaction(2, &action, &oldaction);

//     sigset_t block_set, old_set;

//     sigemptyset(&block_set);
//     sigemptyset(&old_set);

//     sigaddset(&block_set, 2);

//     sigprocmask(SIG_BLOCK, &block_set, &old_set);

//     while (true)
//     {
//         sigset_t pending_set;
//         sigpending(&pending_set);
//         PrintPending(pending_set);
//         sleep(1);
//     }

//     return 0;
// }