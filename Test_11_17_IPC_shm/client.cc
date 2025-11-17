#include "shm.hpp"

int main()
{
    shm client(gpath, guser, 0x666);

    return 0;   
}