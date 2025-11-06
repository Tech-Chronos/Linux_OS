#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <iostream>
#include <string>

#define MAXSIZE 128
#define MAXARGS 32
// shell自己内部维护的第一张表: 命令行参数表
// 故意设计成为全局的
// 命令行参数表
char *gargv[MAXARGS];
int gargc = 0;
const char *gsep = " ";

// 环境变量表
char *genv[MAXARGS];
int genvc = 0;

// 我们shell自己所处的工作路径
char cwd[MAXSIZE];

// 最近一个命令执行完毕，退出码
int lastcode = 0;

// vector<std::string> cmds; // 1000

// ls -a -l > XX.txt -> "ls -a -l" && "XX.txt" && 重定向的方式
// 表明重定向的信息
#define NoneRedir 0
#define InputRedir 1
#define AppRedir 2
#define OutputRedir 3

int redir_type = NoneRedir; // 记录正在执行的执行，重定向方式
char *filename = NULL;      // 保存重定向的目标文件

// 空格空格空格filename.txt
#define TrimSpace(start)        \
    do                          \
    {                           \
        while (isspace(*start)) \
            start++;            \
    } while (0)

void LoadEnv()
{
    // 正常情况，环境变量表内部是从配置文件来的
    // 今天我们从父进程拷贝
    extern char **environ;
    for (; environ[genvc]; genvc++)
    {
        genv[genvc] = (char *)malloc(sizeof(char) * 4096);
        strcpy(genv[genvc], environ[genvc]);
    }
    genv[genvc] = NULL;

    printf("Load env: \n");
    for (int i = 0; genv[i]; i++)
        printf("genv[%d]: %s\n", i, genv[i]);
}
static std::string rfindDir(const std::string &p)
{
    if (p == "/")
        return p;
    const std::string psep = "/";
    auto pos = p.rfind(psep);
    if (pos == std::string::npos)
        return std::string();
    return p.substr(pos + 1); // /home/whb
}

const char *GetUserName()
{
    char *name = getenv("USER");
    if (name == NULL)
        return "None";
    return name;
}

const char *GetHostName()
{
    char *hostname = getenv("HOSTNAME");
    if (hostname == NULL)
        return "None";
    return hostname;
}
const char *GetPwd()
{
    char *pwd = getenv("PWD");
    // char *pwd = getcwd(cwd, sizeof(cwd));
    if (pwd == NULL)
        return "None";
    return pwd;
}

void PrintCommandLine()
{
    printf("[%s@%s %s]# ", GetUserName(), GetHostName(), rfindDir(GetPwd()).c_str()); // 用户名 @ 主机名 当前路径
    fflush(stdout);
}

int GetCommand(char commandline[], int size)
{
    if (NULL == fgets(commandline, size, stdin))
        return 0;
    // 2.1 用户输入的时候，至少会摁一下回车\n abcd\n ,\n '\0'
    commandline[strlen(commandline) - 1] = '\0';
    return strlen(commandline);
}

// ls -a -l >> filenamel.txt -> ls -a -l \0\0 filename.txt
// ls -a -l > XX.txt || ls -a -l >> XX.txt || cat < log.txt || ls -a -l
void ParseRedir(char commandline[])
{
    redir_type = NoneRedir;
    filename = NULL;
    char *start = commandline;
    char *end = commandline + strlen(commandline);
    while (start < end)
    {
        if (*start == '>')
        {
            if (*(start + 1) == '>')
            {
                // 追加重定向
                *start = '\0';
                start++;
                *start = '\0';
                start++;
                TrimSpace(start); // 去掉左半部分的空格
                redir_type = AppRedir;
                filename = start;
                break;
            }
            // 输出重定向
            *start = '\0';
            start++;
            TrimSpace(start);
            redir_type = OutputRedir;
            filename = start;
            break;
        }
        else if (*start == '<')
        {
            // 输入重定向
            *start = '\0';
            start++;
            TrimSpace(start);
            redir_type = InputRedir;
            filename = start;
            break;
        }
        else
        {
            // 没有重定向
            start++;
        }
    }
}

int ParseCommand(char commandline[])
{
    gargc = 0;
    memset(gargv, 0, sizeof(gargv));
    // ls -a -l
    // 故意 commandline : ls
    gargv[0] = strtok(commandline, gsep);
    while ((gargv[++gargc] = strtok(NULL, gsep)))
        ;

    //    printf("gargc: %d\n", gargc); // ?
    //    int i = 0;
    //    for(; gargv[i]; i++)
    //        printf("gargv[%d]: %s\n", i, gargv[i]);
    return gargc;
}

// retunr val:
// 0 : 不是内建命令
// 1 : 内建命令&&执行完毕
int CheckBuiltinExecute()
{
    if (strcmp(gargv[0], "cd") == 0)
    {
        // 内建命令
        if (gargc == 2)
        {
            // 新的目标路径: gargv[1]
            // 1. 更改进程内核中的路径
            chdir(gargv[1]);
            // 2. 更改环境变量
            char pwd[1024];
            getcwd(pwd, sizeof(pwd));                  // /home/whb
            snprintf(cwd, sizeof(cwd), "PWD=%s", pwd); // cwd: PWD=/home/home
            putenv(cwd);
            lastcode = 0;
        }
        return 1;
    }
    else if (strcmp(gargv[0], "echo") == 0) // cd , echo , env , export 内建命令
    {
        if (gargc == 2)
        {
            if (gargv[1][0] == '$')
            {
                // $? ? : 看做一个变量名字
                if (strcmp(gargv[1] + 1, "?") == 0)
                {
                    printf("lastcode: %d\n", lastcode);
                }
                else if (strcmp(gargv[1] + 1, "PATH") == 0)
                {
                    // 不准你用getenv和putenv
                    printf("%s\n", getenv("PATH")); // putenv 和 getenv 究竟是什么, 访问环境变量表！
                }
                lastcode = 0;
            }
            return 1;
            // echo helloworld
            // echo $?
        }
    }

    return 0;
}

int ExecuteCommand()
{
    // 能不能让你的bash自己执行命令：ls -a -l
    pid_t id = fork();
    if (id < 0)
        return -1;
    else if (id == 0)
    {
        // printf("我是子进程，我是exec启动前: %dp\n", getpid());
        //  子进程: 如何执行, gargv, gargc
        //  ls -a -l
        int fd = -1;
        if (redir_type == NoneRedir)
        {
            // Do Nothing
        }
        else if (redir_type == OutputRedir)
        {
            // 子进程要进行输出重定向
            fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            dup2(fd, 1);
        }
        else if (redir_type == AppRedir)
        {
            fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
            dup2(fd, 1);
        }
        else if (redir_type == InputRedir)
        {
            fd = open(filename, O_RDONLY);
            dup2(fd, 0);
        }
        else
        {
            // bug??
        }
        execvpe(gargv[0], gargv, genv);
        exit(1);
    }
    else
    {
        // 父进程
        int status = 0;
        pid_t rid = waitpid(id, &status, 0);
        if (rid > 0)
        {
            lastcode = WEXITSTATUS(status);
            // printf("wait child process success!\n");
        }
    }
    return 0;
}

int main()
{
    // 0. 从配置文件中获取环境变量填充环境变量表的
    // LoadEnv();
    char command_line[MAXSIZE] = {0};
    while (1)
    {
        // 1. 打印命令行字符串
        PrintCommandLine();
        // 2. 获取用户输入
        if (0 == GetCommand(command_line, sizeof(command_line)))
            continue;

        // printf("%s\n", command_line);
        //  ls -a -l > XX.txt || ls -a -l >> XX.txt || cat < log.txt || ls -a -l
        //  ls -a -l > XX.txt -> "ls -a -l" && "XX.txt" && 重定向的方式
        ParseRedir(command_line);
        // printf("command: %s\n", command_line);
        // printf("redir type: %d\n", redir_type);
        // printf("filename: %s\n", filename);

        // 4. 解析字符串 -> "ls -a -l" -> "ls" "-a" "-l" 命令行解释器，就要对用户输入的命令字符串首先进行解析！
        ParseCommand(command_line);

        // 5. 这个命令，到底是让父进程bash自己执行(内建命令)？还是让子进程执行
        if (CheckBuiltinExecute()) // > 0
        {
            continue;
        }

        // 6. 让子进程执行这个命令
        ExecuteCommand();
    }

    return 0;
}