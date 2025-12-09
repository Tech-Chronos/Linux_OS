#include "TcpEchoServer.hpp" // 会话层
#include "IOService.hpp"     // 表示层
#include "NetCal.hpp"        // 应用层

// ./TcpEchoSever port
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        LOG(INFO, "USAGE METHOD -> %s PORT", argv[0]);
        exit(-1);
    }

    uint16_t port = std::stoi(argv[1]);

    auto cal = std::make_shared<Calculator>();
    business_task process_func = std::bind(&Calculator::Calculate, cal.get(), std::placeholders::_1);

    auto ios = std::make_shared<IOService>(process_func);
    task_t io_task = std::bind(&IOService::IOExcute, ios.get(), std::placeholders::_1);

    TcpEchoServer server(port, io_task);
    server.Start();

    return 0;
}