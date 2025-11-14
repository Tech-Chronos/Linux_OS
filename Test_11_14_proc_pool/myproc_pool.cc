#include "process_pool.hpp"

/// @brief 父进程创建信道的函数
/// @param channels 外部传递的信道，vector保存
/// @param id 用来指知道子进程id
/// @param pipenum 管道数量
void CreateChannel(std::vector<Channel> &channels, int pipenum)
{
    int pipefd[2] = {0};
    for (int i = 0; i < pipenum; ++i)
    {
        pipe(pipefd);
        // 4. 子进程获取信道读文件描述符
        pid_t id = fork();

        if (id == 0)
        {
            // 子进程关闭写文件描述符
            close(pipefd[1]);

            // 7. 子进程执行任务
            // 7.1 首先需要获取任务的id
            // 7.2 然后再执行
            //ExcuteTask(taskid);
            GetFuncIdAndExcute(pipefd[0]);
            

            // 子进程执行完直接退出
            exit(0);
        }
        else
        {
            // 现在父进程已经知道了子进程的id和写文件描述符
            channels.emplace_back(pipefd[1], id);

            // 5. 父进程关闭读文件描述符
            close(pipefd[0]);

            std::cout << pipefd[1] << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    // 1. 命令行输入要几个管道的数量
    int pipenum = 0;
    if (argc == 1)
    {
        std::cout << "usage method -> " << argv[0] << " [PIPENUMBER]" << std::endl;
        exit(1);
    }
    else
    {
        pipenum = std::stoi(argv[1]);
    }

    // 2. 管理信道，装载任务
    std::vector<Channel> channels;

    LoadTask();

    // 3. 父进程创建管道

    CreateChannel(channels, pipenum);

    // 6. 向信道中分配任务
    // 先分配五次任务
    AllocTask(channels, 5);

    CompleteAndQuit(channels);

    return 0;
}