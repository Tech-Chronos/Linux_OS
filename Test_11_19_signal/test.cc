#include <iostream>
#include <functional>

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

void handler(int sign)
{
    sleep(1);
    std::cout << "this is a handler function, the signal " << sign <<  std::endl;
}

int main()
{
    // int n = alarm(2);
    // std::cout << "n = " << n << std::endl;
    signal(11, handler);
    while (true)
    {
        int* ptr = nullptr;
        *ptr = 100;
    }

    return 0;
}


// void handler(int sign)
// {
//     sleep(1);
//     std::cout << "this is a handler function, the signal " << sign <<  std::endl;
// }

// int main()
// {
//     // int n = alarm(2);
//     // std::cout << "n = " << n << std::endl;
//     signal(8, handler);
//     while (true)
//     {
//         // 只会输出一次i，因为每次进程都不会，会放到寄存器flag中，每次CPU都会报错，OS通知进程
//         static int i  = 0;
//         std::cout << "i = " << i << std::endl;
//         int a = 10;
//         int res = a / 0;
//     }

//     return 0;
// }

//long long cnt = 0;
// void handler(int sign)
// {
//     //sleep(1);
//     std::cout << "this is a handler function, the signal " << sign <<  " has been replaced!" << ", cnt = " << cnt <<  std::endl;
//     exit(0);
// }

// int main()
// {
//     // signal表示进程收到多少号信号会做handler
//     //signal(2, handler);
//     // 不能被替换的信号
//     //signal(15, handler);

//     // 闹钟响了就退出
//     signal(14, handler);
//     alarm(5);
//     sleep(3);

//     int n = alarm(5);
//     std::cout << "n = " << n << std::endl;
//     while (true)
//     {
//         ++cnt;
//         //std::cout << "Hello Signal, " << "my pid = " << getpid() << std::endl;
//         //sleep(1);
//     }
//     return 0;
// }