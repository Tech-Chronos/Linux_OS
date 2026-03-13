#include <iostream>
#include "BlockingQueue.h"
#include "RingQueue.h"

using task_t = std::function<void(void)>;

void DownloadTask()
{
    std::cout << "I am a download task!" << std::endl;
}

void SQLTask()
{
    std::cout << "I am a SQL task!" << std::endl;
}

void DeleteTask()
{
    std::cout << "I am a Delete task!" << std::endl;
}

void SocketTask()
{
    std::cout << "I am a Socket Task!" << std::endl;
}

task_t tasks[] = {DownloadTask, SQLTask, DeleteTask, SocketTask};


int main()
{

    return 0;
}
