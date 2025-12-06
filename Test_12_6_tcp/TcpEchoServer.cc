#include "TcpEchoServer.hpp"

// ./TcpEchoSever port
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        LOG(INFO, "USAGE METHOD -> %s PORT", argv[0]);
        exit(-1);
    }

    uint16_t port = std::stoi(argv[1]);

    std::unique_ptr<TcpEchoServer> server_ptr = std::make_unique<TcpEchoServer>(port);

    server_ptr->Init();

    server_ptr->Start();
    return 0;
}