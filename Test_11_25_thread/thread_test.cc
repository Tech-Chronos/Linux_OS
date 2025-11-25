#include <iostream>

#include <unistd.h>
#include <pthread.h>

void* func(void*)
{   
    while(true)
    {
        std::cout << "I am a thread, my pid = " << getpid() << "." << std::endl; 
        sleep(1);
    }
}

int main()
{
    pthread_t tid;
    int ret = pthread_create(&tid, nullptr, func, nullptr);

    std::cout << "ret =" << ret << ", tid = " << tid << std::endl;
    while(true)
    {
        std::cout << "I am a main thread, my pid = " << getpid() << "." << std::endl; 
        sleep(1);
    }
    return 0;
}