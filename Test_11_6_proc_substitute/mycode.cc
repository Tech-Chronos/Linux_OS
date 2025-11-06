#include <iostream>
#include <cstdio>
#include <unistd.h>

using namespace std;


int main(int argc, char* argv[], char** env)
{
    cout << "this is a cpp program, my pid = " << getpid() << endl;
    cout << "this is a cpp program, my pid = " << getpid() << endl;
    cout << "this is a cpp program, my pid = " << getpid() << endl;
    cout << "this is a cpp program, my pid = " << getpid() << endl;
    cout << "this is a cpp program, my pid = " << getpid() << endl;

    cout << "--------------------------------------------" << endl;

    for (int i = 0; argv[i]; ++i)
    {
        printf("argv[%d] -> %s\n", i, argv[i]);
    }

    cout << "--------------------------------------------" << endl;
    for (int i = 0; env[i]; ++i)
    {
        printf("env[%d] -> %s\n", i, env[i]);
    }
    
    cout << "--------------------------------------------" << endl;

    return 0;
}