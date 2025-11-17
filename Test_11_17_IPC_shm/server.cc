#include "shm.hpp"

int main()
{
    shm server(gpath, gcreator, 6);

    char* addr = (char*)server.GetShaddr();

    server.Zero();

    while (true)
    {
        std::cout <<  "shm memory content: " << addr << std::endl;
        sleep(1);
    }
    return 0;   
}