#ifndef __NAMEDPIPE__
#define __NAMEDPIPE__
#include <iostream>
#include <string>

#include <cstdio>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// write or read
#define WRITE 1
#define READ 2

#define SIZE 1024

// 要制定管道的名称
const char *gpathname = "/home/dsj/linux_study/Linux_OS/Test_11_17_IPC_namedpipe/myfifo";
const mode_t gmode = 0666;

// 身份
const int creator = 1;
const int user = 2;

// 命名管道，两个毫不相关的进程可以进行进程间通信
// 一个进程负责创建管道文件（需要特定的路径，因为要以读写的方式打开同一个文件）
// 但是还是会创建两个struct file
// 由于是同一个文件，所以OS不会创建两个内核缓冲区，所以能进行读写
class namedpipe
{
private:
    int CreateNamedPipeCommon()
    {
        return mkfifo(_pathname.c_str(), _mode);
    }

    bool OpenPipeCommon(mode_t mode)
    {
        _fd = open(_pathname.c_str(), mode);
        if (_fd < 0)
            return false;
        else
            return true;
    }

public:
    namedpipe(int identification, int wrore, const char *pathname = gpathname, mode_t mode = gmode)
        : _pathname(pathname), _mode(mode), _identification(identification), _wrore(wrore)
    {
    }

    // 只有一个进程负责创建
    int CreateNamedPipe()
    {
        if (_identification == creator)
        {
            return CreateNamedPipeCommon();
        }
        else
            return 1;
    }

    bool OpenPipeForRead()
    {
        return OpenPipeCommon(O_RDONLY);
    }

    bool OpenPipeForWrite()
    {
        return OpenPipeCommon(O_WRONLY);
    }

    int Write()
    {
        int ret = 0;
        if (_wrore == WRITE)
        {
            std::string message;
            std::getline(std::cin, message);

            ret = write(_fd, message.c_str(), message.size());
            
            if (ret < 0)
            {
                std::cout << "write error !" << std::endl;
                return ret;
            }
            if (ret == 0)
            {
                std::cout << "read close" << std::endl;
                return ret;
            }
        }
        return ret;
    }

    int Read()
    {
        int ret = 0;
        if (_wrore == READ)
        {
            char buffer[SIZE];

            ret = read(_fd, buffer, sizeof(buffer));

            if (ret > 0)
            {
                buffer[ret] = '\0';
                std::cout << buffer << std::endl;
            }
            if (ret < 0)
            {
                std::cout << "write fd close, I would close too !" << std::endl;
                return ret; 
            }
            
        }
        return ret;
    }

    ~namedpipe()
    {
        if (_identification == creator)
        {
            unlink(_pathname.c_str());
        }
        close(_fd);
    }

private:
    std::string _pathname;
    mode_t _mode;

    int _identification;
    int _fd;

    int _wrore; // 读还是写
};

#endif
