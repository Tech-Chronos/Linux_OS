#include <iostream>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

void Run()
{
    while (1)
    {
        cout << "I am the child process and my pid is: " << getpid() << endl;
        sleep(1); 
    }
}

int main()
{
    const int num = 5;

    for (size_t i = 0; i < num; ++i)
    {
        pid_t id = fork();
        if (id == 0)
        {
            Run();
        }
    }
    while (1)
    {
        cout << "I am the father process and my pid is: " << getpid() << endl;
        sleep(1);
    }
    return 0;
}

// int main()
// {
//     pid_t id = fork();

//     if (id == 0)
//     {
//         while (1)
//         {
//             cout << "I am the child process ! My pid is : " << getpid() << " and my ppid is : " << getppid() << endl; 
//             sleep(1);
//         }
//     }
//     else if (id == -1)
//     {
//         cout << "new child process failture!" << endl;
//     }
//     else
//     {
//         while (1)
//         {
//             cout << "I am the parent process ! My pid is : " << getpid() << " and my ppid is : " << getppid() << endl; 
//             sleep(1);
//         }
//     }
//     return 0;
// }
