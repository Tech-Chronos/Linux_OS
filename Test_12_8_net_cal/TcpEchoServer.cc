#include "TcpEchoServer.hpp"
#include "Command.hpp"

// ./TcpEchoSever port
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        LOG(INFO, "USAGE METHOD -> %s PORT", argv[0]);
        exit(-1);
    }

    uint16_t port = std::stoi(argv[1]);

    DealCommand command;

    task_t task = std::bind(&DealCommand::GetCommand, &command, std::placeholders::_1, std::placeholders::_2);

    std::unique_ptr<TcpEchoServer> server_ptr = std::make_unique<TcpEchoServer>(port, task);

    server_ptr->Init();

    server_ptr->Start();
    return 0;
}