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
const int gcreator = 1;
const int guser = 2;

// 路径
std::string gpath = "./";

// 首先就得获取共享内存 shmget
class shm
{
private:
    void CreateKey()
    {
        _key = ftok(_path.c_str(), _proj_id);
    }

    int GetShmid(int size, int flag)
    {
        int shmid = shmget(_key, size, flag);
        return shmid;
    }

    static std::string ToHex(int key)
    {
        char buffer[1024];

        snprintf(buffer, sizeof(buffer), "0x%x", key);

        // std::cout << buffer << std::endl;

        return buffer;
    }

public:
    shm(const std::string &path, int identification, int proj_id)
        : _path(path), _idetification(identification), _proj_id(proj_id)
    {
        CreateKey();
        std::cout << "key -> " << ToHex(_key) << std::endl;

        if (_idetification == gcreator)
        {
            GetShmidForCreate();
        }
        else if (_idetification == guser)
        {
            GetShmidForUse();
        }
    }

    bool GetShmidForCreate()
    {
        if (_idetification == gcreator)
        {
            _shmid = GetShmid(SIZE, IPC_CREAT | IPC_EXCL);
            if (_shmid >= 0)
            {
                std::cout << "shmid -> " << _shmid << std::endl;
                return true;
            }
        }

        return false;
    }

    bool GetShmidForUse()
    {
        if (_idetification == guser)
        {
            _shmid = GetShmid(SIZE, IPC_CREAT);
            if (_shmid >= 0)
            {
                std::cout << "shmid -> " << _shmid << std::endl;
                return true;
            }
        }
        return false;
    }

    ~shm()
    {
        if (_idetification == gcreator)
        {
            sleep(5);
            int ret = shmctl(_shmid, IPC_RMID, nullptr);
            if (ret == 0)
            {
                std::cout << "creator quit, shm has deleted !" << std::endl;
            }
            else
            {
                std::cout << "shmctl error !" << std::endl;
            }
        }
        else
            std::cout << "user quit, but shm has not deleted !" << std::endl;
    }

private:
    const std::string _path;
    int _idetification;
    int _proj_id;
    key_t _key;

    int _shmid;
};

#endif
