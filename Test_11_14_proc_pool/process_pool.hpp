#ifndef PROCESS_POOL

#define PROCESS_POOL
#include <iostream>
#include <vector>
#include <functional>

#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TASK_SIZE 3

std::function<void()> func[TASK_SIZE];

class Channel
{
public:
    Channel() = default;

    Channel(int wfd, int childprocid)
        : _wfd(wfd), _childprocid(childprocid)
    {
    }

    int GetWfd()
    {
        return _wfd;
    }

    pid_t GetChildprocid()
    {
        return _childprocid;
    }

    // 当父进程写fd关闭，子进程就会知道，就会推出
    void CloseWfd()
    {
        int ret = close(_wfd);
        if (ret == 0)
        {
            std::cout << "close success !" << std::endl;
        }
    }

    void WaitChild()
    {
        int rid = waitpid(_childprocid, nullptr, 0);
        if (rid > 0)
        {
            std::cout << "child quit success !" << std::endl;
        }
    }

private:
    int _wfd;
    pid_t _childprocid;
};

class task
{
public:
    static void print_task()
    {
        std::cout << "I am a print_task !" << std::endl;
    }

    static void load_task()
    {
        std::cout << "I am a load_task !" << std::endl;
    }

    static void sql_task()
    {
        std::cout << "I am a sql_task !" << std::endl;
    }
};

// 装载任务
void LoadTask()
{
    srand(time(nullptr) ^ getpid() ^ 7777);

    func[0] = std::bind(&task::print_task);
    func[1] = std::bind(&task::load_task);
    func[2] = std::bind(&task::sql_task);
}

// 分配一次的函数
void AllocTaskOnce(std::vector<Channel> &channels)
{
    static int index = 0;
    sleep(1);
    int taskid = random() % TASK_SIZE;

    // 按顺序挑选进程
    int writefd = channels[index].GetWfd();
    // 面向字节流
    int ret = write(writefd, &taskid, sizeof(taskid));
    if (ret < 0)
    {
        std::cerr << "write error!" << std::endl;
    }

    index = (index + 1) % channels.size();
}

// 父进程分配任务，可以传递次数
void AllocTask(std::vector<Channel> &channels, int times = -1)
{
    if (times > 0)
    {
        while (times--)
        {
            AllocTaskOnce(channels);
        }
    }
    else
    {
        while (true)
        {
            AllocTaskOnce(channels);
        }
    }
}

// 子进程执行任务
void ExcuteTask(int taskid)
{
    std::cout << "I am a worker, my id = " << getpid() << ", I am doing task " << taskid << std::endl;
    func[taskid]();
}

// 获取任务ID
void GetFuncIdAndExcute(int rfd)
{
    int taskid;
    while (true)
    {
        int ret = read(rfd, &taskid, sizeof(taskid));
        if (ret == 0)
        {
            // 父进程关闭写端，管道结束
            std::cout << "child " << getpid() << " exit" << std::endl;
            break;
        }
        else if (ret < 0)
        {
            std::cerr << "read error!" << std::endl;
            break;
        }
        ExcuteTask(taskid);
    }
}

void CompleteAndQuit(std::vector<Channel> &channels)
{
    for (auto &channel : channels)
    {
        channel.CloseWfd();
    }
    for (auto &channel : channels)
    {
        channel.WaitChild();
    }
}

#endif