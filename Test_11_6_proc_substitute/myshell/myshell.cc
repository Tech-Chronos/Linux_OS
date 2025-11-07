#include <iostream>

#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE 512
#define DELIM " "

using namespace std;

// 定义全局的命令行参数数组。
char *argv[SIZE] = {nullptr};

// 定义全局的数组存放命令
char command[SIZE] = {0};

// 定义全局的env数组
char* env[SIZE * 4];

// 定义退出码
int lastcode;

void LoadEnv()
{
    // bash是从配置文件导入的，这里因为是模仿，所以从父进程bash导入
    extern char** environ;
    int i = 0;
    for (i = 0; environ[i]; ++i)
    {
        env[i] = environ[i];
    }
    
    env[i] = nullptr;
    // for (int j = 0; env[j]; ++j)
    // {
    //     cout << env[j] << endl;
    // }
}

char *SplitPath(char *cwd)
{
    char *tail = cwd;
    while (*tail != '\0')
    {
        ++tail;
    }
    while (*tail != '/')
    {
        --tail;
    }
    return tail;
}

const char *GetUser()
{
    const char *user = getenv("USER");
    return user;
}

const char *GetHostName()
{
    return "ali-cloud";
}

char *GetCWD()
{
    char *pwd = getenv("PWD");
    return pwd;
}

const char *GetHome()
{
    const char *home = getenv("HOME");
    return home;
}

void GetCommandLineRemind()
{
    char buffer[SIZE] = {0};
    const char *user = GetUser();
    const char *hostname = GetHostName();
    char *cwd = GetCWD();

    char *split = SplitPath(cwd);
    snprintf(buffer, SIZE, "[%s@%s:%s]$ ", user, hostname, (strlen(split) == 1) ? "/" : (++split));

    printf("%s", buffer);
}

void GetUserCommand()
{
    // 从标准输入读一行数据
    fgets(command, SIZE, stdin);

    size_t len = strlen(command);
    command[len - 1] = '\0';
    // cout << buffer << endl;
}

void SplitCommand()
{
    argv[0] = strtok(command, DELIM);
    int index = 1;
    while ((argv[index++] = strtok(nullptr, DELIM)))
        ;

    // for (int i = 0; argv[i]; ++i)
    // {
    //     printf("argv[%d] -> %s\n", i, argv[i]);
    // }
}

void InnerCd()
{
    // 1. 首先根据命令行参数的第二个参数找到需要改变的工作路径（绝对路径）
    // 2. 因为下次打印命令行提示符的时候需要根据当前的环境变量找到PWD，所以需要更改
    // 3. 所以首先根据系统调用接口 getcwd() 找到当前的工作路径（不能直接拿命令行参数，因为 .. & .）
    // 4. 然后定义一个全局或者静态或者堆上的空间，因为需要调用putenv，当然setenv就不需要
    const char *path = ((argv[1] == nullptr) ? GetHome() : argv[1]);

    chdir(path);

    char tmp[SIZE];
    getcwd(tmp, SIZE);

    char* cwd = (char*)malloc(sizeof(char) * SIZE);
    snprintf(cwd, SIZE, "PWD=%s", tmp);

    putenv(cwd);
}

int CheckBuiltin()
{
    int yes = 0;
    if (strcmp(argv[0], "cd") == 0)
    {
        yes = 1;
        if (argv[2] == nullptr)
        {
            InnerCd();
        }
        else
        {
            cout << "unkonwn command" << endl;
        }
    }
    else if (strcmp(argv[0], "echo") == 0)
    {
        yes = 1;
        if (*(*(argv + 1) + 0) == '$')
        {
            if (*(*(argv + 1) + 1) == '?')
            {
                cout << lastcode << endl;
                lastcode = 0;
            }
            else
            {
                // 1. 先分割出命令行参数中要获取的环境变量
                char buffer[SIZE] = { 0 };
                snprintf(buffer, SIZE, "%s", (*(argv + 1) + 1));

                //cout << buffer << endl;

                // 2. 在环境变量表中 找到这个环境变量
                int i = 0;
                for (; env[i]; ++i)
                {
                    char* stay = *(env + i);
                    char* head = *(env + i);
                    while (*head != '=')
                    {
                        ++head;
                    }

                    // 找到 = 前面的字符
                    char prestr[SIZE] = { 0 };
                    strncpy(prestr, stay, head - stay);
                    prestr[head - stay] = '\0';

                    //printf("%s\n",prestr);
                    if (strcmp(prestr, buffer) == 0)
                    {
                        cout << env[i] << endl;
                        break;
                    }
                }
                if (env[i] == nullptr)
                {
                    cout << "this env is illegal" << endl;
                }

            }
        }
        else
        {
            cout << "please input [echo $env]" << endl;
        }
    }
    return yes;
}

void ExcuteCommand()
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // 子进程
        execvp(argv[0], argv);
        exit(errno);
    }
    else
    {
        int status = 0;
        int rid = waitpid(pid, &status, 0);
        if (rid > 0)
        {
            // 等待成功
            if (WIFEXITED(status) == 0)
            {
                lastcode = WEXITSTATUS(status);
                if (lastcode != 0)
                {
                    cout << argv[0] << ": " << strerror(lastcode) << ": " << lastcode << endl;
                }
            }
        }
    }
}

int main()
{
    int quit = 0;
    while (!quit)
    {
        LoadEnv();

        // 1. 打印命令行提示字符串
        GetCommandLineRemind();

        // 2. 获取用户命令
        GetUserCommand();

        // 3. 分割命令行参数
        SplitCommand();

        // 4. 检查是否是内建命令
        int ret = CheckBuiltin();
        if (ret)
            continue;

        // 5. 执行命令
        ExcuteCommand();
    }
    return 0;
}