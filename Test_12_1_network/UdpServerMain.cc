#include "UdpServer.hpp"

int main()
{
    std::unique_ptr<UdpServer> usptr = std::make_unique<UdpServer>();

    usptr->Init();

    usptr->Start();
    return 0;
}