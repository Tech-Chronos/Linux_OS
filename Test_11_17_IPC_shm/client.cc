#include "shm.hpp"

int main()
{
    shm process1(creator);

    int ret = process1.CreateShm();
    if (ret  != -1)
    {
        std::cout << "return val -> " << ret << std::endl;
    }
    else
    {
        perror("shm error");
    }
    return 0;   
}