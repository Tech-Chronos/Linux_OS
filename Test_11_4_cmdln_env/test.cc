#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>

using namespace std;

int g_val = 100000;

// 本质是bash中的环境变量可以继承给子进程，就是bash中维护了env的表（全局的)
// 然后子进程可以看到
int main(int argc, char* argv/*, char* environ[]*/)
{
    extern char** environ;
    for (int i = 0; environ[i]; ++i)
    {
        cout << "env[" << i << "] : " << environ[i] << endl;
    }

    cout << "----------------------------------" << endl;

    const char* str = getenv("PATH");

    cout << str << endl;
}


// 父进程的数据子进程能看到
// int main()
// {
//     cout << "I am the parent process, pid: " << getpid() << ", ppid is: " << getppid() << ", g_val = " << g_val << endl;
//     sleep(5);

//     pid_t id = fork();
//     if (id == 0)
//     {
//         cout << "I am the child process, pid: " << getpid() << ", ppid is: " << getppid() << ", g_val = " << g_val << endl;
//         sleep(1);
//     }
//     else
//     {
//         cout << "I am the parent process, pid: " << getpid() << ", ppid is: " << getppid() << ", g_val = " << g_val << endl;
//         sleep(1);
//     }
// }


// 命令行参数：可根据在命令行的输入定制结果
// int main(int argc, char* argv[])
// {
//     if (argc != 2)
//     {
//         cout << "usage method -> " << argv[0] << "-[a b c d]" << endl;
//     }
//     else if (strcmp(argv[1], "-a") == 0)
//     {
//         cout << "func A : " << endl;
//     }
//     else if (strcmp(argv[1], "-b") == 0)
//     {
//         cout << "func B : " << endl;
//     }
//     else if (strcmp(argv[1], "-c") == 0)
//     {
//         cout << "func C : " << endl;
//     }
//     else if (strcmp(argv[1], "-d") == 0)
//     {
//         cout << "func D : " << endl;
//     }
//     else
//     {
//         cout << "we don't have this function!!!" << endl;
//     }
//     return 0;
// }

// 本质是命令行输入的参数首先被bash拿到，而且在命令行中执行的可执行文件都是bash的子进程
// 所以命令行的参数在bash中被解析成argv，然后bash用fork()创建子进程，调用execv()
// 将argv复制到新进程的栈中，最终传给main().

// int main(int argc, char* argv[])
// {
//     for (int i = 0; i < argc; ++i)
//     {
//         printf("argv[%d] -> %s\n", i, argv[i]);
//     }

//     return 0;
// }