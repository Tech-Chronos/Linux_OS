#include <iostream>
#include <string>

#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

const int size = 1024;

std::string DealMessage(std::string message)
{

    static int cnt = 0;
    std::string count_number = std::to_string(cnt);
    ++cnt;

    pid_t id = getpid();
    std::string sid = std::to_string(id);

    message = message + " this is the " + count_number + " time ! " + " my pid = " + sid;

    return message;
}


void ChildProcessWrite(int writefd)
{
    std::string message = "I am child process !";

    int pipe_size = 0;
    while(true)
    {
        std::string info = DealMessage(message);

        // 没有写入 '\0'
        ssize_t ret = write(writefd, info.c_str(), info.size());
        if (ret == -1)
        {
            std::cout << "write error! errno = " << errno << ", strerror = " << strerror(errno) << std::endl;
        }

        //sleep(1);

        // 读进程阻塞
        //sleep(100);

        // //管道大小是 64kb，管道写完，写进程阻塞
        // char* ch = "a";
        // write(writefd, ch, 1);

        // std::cout << "pipefd = " << pipe_size++ << std::endl;
        break;
    }

    // 关掉子进程fd
    close(writefd);
    std::cout << "write process quit!" << std::endl;

}

void FatherProcessRead(int readfd)
{
    char inbuffer[size];
    while (true)
    {
        //sleep(500);
        // 会等子进程写完再读，同步性
        int ret = read(readfd, inbuffer, sizeof inbuffer - 1);
        
        // 读的fd关闭，read就会返回0
        if (ret == 0)
        {
            std::cout << "write fd close" << std::endl;
            break;
        }
        else if (ret != -1)
        {
            // read 放入数组中要写入 0
            inbuffer[ret] = '\0';
            std::cout << inbuffer << std::endl;
        }
        else 
        {
            std::cout << "read error! errno = " << errno << ", strerror = " << strerror(errno) << std::endl;
            break;
        }
    }

}


int main()
{
    // 1. 父进程创建管道文件描述符，默认fd[0] 是读，fd[1] 写
    int pipefd[2] = {0};
    int retval = pipe(pipefd);
    
    if (retval == -1)
    {
        std::cout << "pipe error!" << " errno = " << errno << ", strerror = " << strerror(errno) << std::endl;
    }
    else 
    {
        std::cout << "create pipe success! pipefd[0] = " << pipefd[0] << ", pipefd[1] = " << pipefd[1] << std::endl;
    }
    std::cout << "管道创建成功，等待子进程创建" << std::endl;
    sleep(1);

    //2. 创建子进程
    int pid = fork();

    if (pid == 0)
    {
        // child
        // 3. 子进程在管道中写入，关闭文件描述符 0
        close(pipefd[0]);

        // 4. 子进程写入数据
        ChildProcessWrite(pipefd[1]);
    }
    else
    {
        // father
        // 3. 父进程在管道中读，关闭文件描述符 1
        close(pipefd[1]);

        // 4. 父进程读管道数据
        FatherProcessRead(pipefd[0]);

        // 等待子进程退出
        waitpid(pid, nullptr, 0);
    }

    return 0;
}