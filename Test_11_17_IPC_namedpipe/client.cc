#include "namedpipe.hpp"

int main()
{
    namedpipe writepipe(user, WRITE);

    if (!writepipe.OpenPipeForWrite())
    {
        std::cout << "writepipe.OpenPipeForWrite" << std::endl;
    }

    while (true)
    {
        if (writepipe.Write() == 0)
            break;
    }
    return 0;
}