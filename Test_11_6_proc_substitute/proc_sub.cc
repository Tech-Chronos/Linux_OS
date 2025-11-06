#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;


int main()
{
    extern char** environ;

    pid_t pid = fork();

    if (pid == 0)
    {
        // child
        cout << "child process begin ..." << endl;

        char* const argv[] = 
        {
            (char*)"./mycode",
            (char*)"-l",
            (char*)"-a",
            (char*)"--color",
            nullptr
        };

        char* const envp[] = 
        {
            (char*)"nihao=1111111111111",
            (char*)"hello=2222222222222",
            nullptr
        };
        sleep(1);
        //execl("/usr/bin/ls", "-l", "-a", "--color", nullptr);
        //execv("/usr/bin/ls", argv);
        //execlp("ls", "-l", "-a", "--color", nullptr);
        //execlp("./mycode", "-a", "-b" , "-c" ,nullptr);

        execvpe("./mycode", argv, envp);
        //execvpe("./mycode", argv, environ);
        // 程序替换不执行这个代码，执行到了就替换失败了
        cout << "程序替换失败" << endl;
        exit(1);
    }

    // 父进程
    int status = 0;
    cout << "I am waiting for my child process." << endl;
    pid_t rid = waitpid(pid, &status, 0);
    if (rid > 0)
    {
        if (WIFEXITED(status))
        {
            cout << "waiting success, child pid = " << pid << endl;
            cout << "exit code = " << WEXITSTATUS(status) << ", exit_signal = " << (status & 0x7f) << endl;
        }
        else
        {
            cout << "quit abnormal!" << ", exit_signal = " << (status & 0x7f) << endl;
        }
    }
    if (rid < 0)
    {
        cout << "wait failture..." << endl;
    }
    
    return 0;
}

// int main()
// {
//     cout << "father process begin ..." << endl;

//     execl("/usr/bin/ls", "-l", nullptr);

//     cout << "father process end ..." << endl;

//     return 0;
// }