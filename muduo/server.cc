#include "EchoServer.hpp"


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));

    EchoServer server(port);
    server.Start();

    return 0;
}