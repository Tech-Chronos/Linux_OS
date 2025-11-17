#include "namedpipe.hpp"

int main()
{
    namedpipe readpipe(creator, READ);

    int ret = readpipe.CreateNamedPipe();
    if (ret == 0)
    {
        std::cout << "pipe create successfully !" << std::endl;
    }
    else
    {
        perror("mkfifo");
        return -1;
    }

    if (!readpipe.OpenPipeForRead())
    {
        std::cout << "readpipe.OpenPipeForRead" << std::endl;
    }

    while (true)
    {
        if (readpipe.Read() < 0)
            break;
    }

    
    return 0;
}