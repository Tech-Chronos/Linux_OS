#include "task.h"

typedef void(*func_t)();

//func_t task[N];

// C++11
function<void()> task[N];

void LoadTask()
{
    task[0] = bind(MySQLSync);
    task[1] = bind(LoadLog);
    task[2] = bind(Network);
}


void DoOtherThing()
{
    for (int i = 0; i < 3; ++i)
    {
        task[i]();
    }
}

//有关 waitpid 的第三个参数
void RunTask()
{
    int cnt = 7;
    while (cnt)
    {
        cout << "I am the child process, my pid = " << getpid() << ", my ppid = " << getppid() << ", cnt = " << cnt << endl;
        --cnt;
        sleep(1);
    }
}

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        RunTask();
        cout << "child process quit...." << endl;
    }
    else
    {
        LoadTask();
        while (1)
        {
            int status = 0;
            pid_t id = waitpid(pid, &status, WNOHANG);
            cout << "I am the father process, my pid = " << getpid() << ", my ppid = " << getppid() << ". I am waiting for my child process!"<< endl;
            //usleep(1000);
            if (id > 0)
            {
                // 等待成功
                if (WIFEXITED(status))
                {
                    // 如果退出信号为0，进入
                    cout << "wait success, child process pid = " << id << endl;
                    cout << "WEXITSTATUS = " << WEXITSTATUS(status) << endl;
                }
                else
                {
                    cout << "child process exit abnormal, exit_signal = " << (status & 0x7f) << endl;
                    cout << "exit_code = " << ((status << 16) >> 24) << endl;
                }
                break;
            }
            else if (id == 0)
            {
                // 子进程还没有退出
                cout << "child process has not quited, I can do DoOtherThing() right now!" << endl;
                DoOtherThing();
                //usleep(1000);
            }
            else
            {
                cout << "wait child process failture" << endl;
            }
        }
    }
    return 0;
}

// void RunTask()
// {
//     int cnt = 4;
//     while (cnt)
//     {
//         cout << "I am the child process, my pid = " << getpid() << ", my ppid = " << getppid() << ", cnt = " << cnt << endl;
//         --cnt;
//         sleep(1);
//     }
// }

// int main()
// {
//     pid_t pid = fork();

//     if (pid == 0)
//     {
//         RunTask();
//         cout << "child process quit...." << endl;
//     }
//     else
//     {
//         cout << "I am the father process, my pid = " << getpid() << ", my ppid = " << getppid() << endl;
//         sleep(6);
//         int status = 0;
//         pid_t id = waitpid(pid, &status, 0);
//         if (id > 0)
//         {
//             // 等待成功
//             if (WIFEXITED(status))
//             {
//                 // 如果退出信号为0，进入
//                 cout << "wait success, child process pid = " << id << endl;
//                 cout << "WEXITSTATUS = " << WEXITSTATUS(status) << endl;
//             }
//             else
//             {
//                 cout << "child process exit abnormal, exit_signal = " << (status & 0x7f) << endl;
//                 cout << "exit_code = " << ((status << 16) >> 24) << endl;
//             }
//         }
//         else
//         {
//             cout << "wait child process failture" << endl;
//         }
//     }
//     return 0;
// }