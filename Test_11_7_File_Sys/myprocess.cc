#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int main()
{
    cout << "I am the father process, my pid = " << getpid() << endl;
    sleep(3);
    int fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);

    const char *message = "Hello File System!\n";
    write(fd, message, strlen(message));

    int pid = fork();
    if (pid == 0)
    {
        // child
        while (1)
        {
            // 子进程的 struct file* fd[]（文件描述符表） 以写时拷贝的方式继承自父进程。
            cout << "I am the child process, my pid = " << getpid() << ", I just want to look my fd!" << endl;
            sleep(1);
        }
    }
    else
    {
        // father
        int status = 0;
        int rid = waitpid(pid, &status, 0);

        if (rid > 0)
        {
            if (WIFEXITED(status) == 0)
            {
                int exit_code = WEXITSTATUS(status);
                cout << "wait success and the exit code = " << exit_code << endl;
            }
        }
    }

    close(fd);
    return 0;
}