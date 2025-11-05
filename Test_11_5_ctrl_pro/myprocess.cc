#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <wait.h>

using namespace std;


// 关于waitpid的的二个参数和第三个参数
int main()
{
    cout << "I am the father process, pid = " << getpid() << ", ppid = " << getppid() << endl;
    pid_t id = fork();

    sleep(1);

    if (id == 0)
    {
        int cnt = 3;
        while (cnt)
        {
            cout << "I am the child process, pid = " << getpid() << ", ppid = " << getppid() << endl;
            sleep(1);
        }
        cout << "child process quit!" << endl;
    }
    else
    {
        sleep(4);
        cout << "I am the father process, pid = " << getpid() << ", ppid = " << getppid() << endl;
        
        int status = 0;

        int ret = waitpid(id, &status, 0);

        if (ret > 0)
        {
            cout << "wait success! my child process pid = " << ret << endl;
            cout << "status = " << status << endl;

            cout << "exit_code = " << ((status << 16) >> 24) << endl;
            cout << "exit_signal = " << (status & 0x7f) << endl;
        }
        else
        {
            cout << "wait failture !" << endl;
        }
    }
    return 0;
}


// int main()
// {
//     cout << "I am the father process, pid = " << getpid() << ", ppid = " << getppid() << endl;
//     pid_t id = fork();

//     sleep(1);

//     if (id == 0)
//     {
//         int cnt = 3;
//         while (cnt--)
//         {
//             cout << "I am the child process, pid = " << getpid() << ", ppid = " << getppid() << endl;
//             sleep(1);
//         }
//         exit(1);
//     }
//     else
//     {
//         sleep(4);
//         cout << "I am the father process, pid = " << getpid() << ", ppid = " << getppid() << endl;
        
//         int status = 0;

//         int ret = waitpid(id, &status, 0);

//         if (ret > 0)
//         {
//             cout << "wait success! my child process pid = " << ret << endl;
//             cout << "status = " << status << endl;

//             cout << "exit_code = " << ((status << 16) >> 24) << endl;
//             cout << "exit_signal = " << (status & 0x7f) << endl;
//         }
//         else
//         {
//             cout << "wait failture !" << endl;
//         }
//     }
//     return 0;
// }

// int main()
// {
//     printf("hello");
//     sleep(2);
//     exit(0);

//     return 0;
// }



// void print()
// {
//     int *p = nullptr;
//     *p = 10;
// }

// int main()
// {
//     print();
//     return 0;
// }


/*
   可以自己定义错误信息（退出码）
*/
// enum 
// {
//     success,
//     div_zero,
//     mod_zero
// };

// int exit_code = success;

// double div(double left, double right)
// {
//     if (right == 0)
//     {
//         exit_code = div_zero;
//         return exit_code;
//     }
//     else 
//         return left / right;
// }

// int main()
// {
//     double l1 = 10, l2 = 5;

//     double r1 = 5, r2 = 2.5;

//     double excep = 0;

//     cout << l1 << " / " << r1 << " = " << div(l1, r1) << endl;

//     cout << l2 << " / " << r2 << " = " << div(l2, r2) << endl;
    
//     cout << l1 << " / " << excep << " = " << div(l1, excep) << endl;

//     cout << exit_code << endl;

//     return exit_code;
// }

// int main()
// {
//     // 查看系统所有的 exit_code
//     for (int i = 0; i < 100; ++i)
//     {
//         cout << "i[" << i << "] -> " << strerror(i) << endl;
//     }
//     return 0;
// }