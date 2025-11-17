#ifndef __SHM__
#define __SHM__

#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// 定义共享内存大小
#define SIZE 4096

// 定义角色
const int creator = 1;
const int user = 2;

// 路径
std::string gpath = "./";

key_t gkey = ftok(gpath.c_str(), 678);

// 首先就得获取共享内存 shmget
class shm
{
public:
    shm(int identification, key_t key = gkey)
        : _idetification(identification), _key(key)
    {
    }

    int CreateShm()
    {
        int ret = 0;
        std::cout << _key << std::endl;
        ret = shmget(_key, SIZE, IPC_CREAT | IPC_EXCL);
        _shmid = ret;
        return ret;
    }

private:
    int _idetification;
    key_t _key;

    int _shmid = -1;
};

#endif
