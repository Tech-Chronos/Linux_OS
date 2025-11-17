#include "shm.hpp"

int main()
{
    shm client(gpath, guser, 6);

    char* addr = (char*)client.GetShaddr();

    std::cout << addr << std::endl;

    std::cout << client.GetShaddr() << std::endl;
    
    char character = 'a';

    client.Zero();

    while (character < 'z')
    {
        addr[character - 'a'] = character;
        sleep(2);
        ++character;
    }

    return 0;   
}