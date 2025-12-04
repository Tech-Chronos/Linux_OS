#include "TcpEchoServer.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        LOG(INFO, "USAGE METHOD -> %s PORT", argv[0]);
        exit(-1);
    }

    std::unique_ptr<TcpServer> ts_ptr = std::make_unique<TcpServer>(std::stoi(argv[1]));
    ts_ptr->Init();
    ts_ptr->Start();
    return 0;
}