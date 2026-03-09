//
// Created by dsj on 2026/3/9.
//
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/shm.h>

using namespace std;

int GetKey(const char* path)
{
    return ftok(path, 1);
}

int GetShm(int key, int size)
{
    int shmid = shmget(key, size, IPC_CREAT | IPC_EXCL);
    return shmid;
}

void* At(int shmid)
{
    return shmat(shmid, nullptr, 0);
}

void Detach(void* addr)
{
    shmdt(addr);
}

void Delete(int shmid)
{
    shmctl(shmid, IPC_RMID, nullptr);
}

int main()
{
    char path[1024];

    getcwd(path, sizeof(path));

    int key = GetKey(path);

    int shmid = GetShm(key, 4096);

    cout << "key: " << key << endl;
    cout << "shmid: " << shmid << endl;

    Delete(shmid);
    return 0;
}