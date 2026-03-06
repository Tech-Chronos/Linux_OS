//
// Created by dsj on 2026/3/5.
//

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

void parent()
{
    std::cout << "I am father, my pid = " << getpid() << "wait child exit ..." << std::endl;
    sleep(1);
}

int main(int argc, char* argv[], char* environ[])
{
    pid_t id = fork();

    if (id == 0)
    {
        //execl("/usr/bin/which", "which", "ls", nullptr);
        char* arg[]
        {
            "ls",
            "-l",
            "-a",
            "--color",
            nullptr
        };
        //execv("/usr/bin/ls", arg);

        //execvp("ls", arg);

        //execle("/usr/bin/ls", "ls", "-l", nullptr, environ);
        //execvpe("ls", arg, environ);

        execve("/usr/bin/ls", arg, environ);
        std::cout << "execl error!" << std::endl;
    }
    else
    {
        int status;
        while (true)
        {
            pid_t pid = waitpid(id, &status, 0);
            if (WIFEXITED(status))
            {
                std::cout << "child quit: " << id << ", ";
                std::cout << "exit code = " << WEXITSTATUS(status) << std::endl;
            }
            break;
        }
    }

    return 0;
}