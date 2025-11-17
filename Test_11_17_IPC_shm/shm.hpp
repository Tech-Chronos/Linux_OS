#ifndef __SHM__
#define __SHM__

#include <iostream>
#include <string>

#include <cstring>

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

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

    std::string RoleToString(int id)
    {
        if (id == gcreator)
            return "creator";
        else if (id == guser)
            return "user";
        else
            return "none";
    }

    // attach shm 到虚拟地址
    void *ShmAt()
    {
        if (_shaddr != nullptr)
            Detach(_shaddr);
        void *ptr = shmat(_shmid, nullptr, 0);
        if (ptr == (void*)-1)
        {
            perror("shmat error:");
            exit(-1);
        }

        std::cout << "_identification: " << RoleToString(_idetification) << " attach shm..." << std::endl;
        return ptr;
    }

    // detach
    int Detach(void *shaddr)
    {
        int ret = shmdt(shaddr);
        if (ret < 0)
        {
            perror("detach error:");
        }
        return ret;
    }

public:
    shm(const std::string &path, int identification, int proj_id)
        : _path(path), _idetification(identification), _proj_id(proj_id), _shaddr(nullptr)
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

        _shaddr = ShmAt();
    }

    bool GetShmidForCreate()
    {
        if (_idetification == gcreator)
        {
            _shmid = GetShmid(SIZE, IPC_CREAT | IPC_EXCL | 0666);
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
            _shmid = GetShmid(SIZE, IPC_CREAT | 0666);
            if (_shmid >= 0)
            {
                std::cout << "shmid -> " << _shmid << std::endl;
                return true;
            }
        }
        return false;
    }

    void *GetShaddr()
    {
        return _shaddr;
    }

    void Zero()
    {
        if (_shaddr)
        {
            memset(_shaddr, 0, SIZE);
        }
    }

    ~shm()
    {
        Detach(_shaddr);
        if (_idetification == gcreator)
        {
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
    const std::string _path; // 路径 (shmget 的第二个参数)
    int _idetification;      // 身份（创建或者使用）
    int _proj_id;            // 创建key的项目id
    key_t _key;              // shmget的第一个参数val

    int _shmid;

    void *_shaddr; // 获得挂载到虚拟内存的地址
};

#endif
