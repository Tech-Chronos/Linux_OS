#include "TcpEchoServer.hpp"
#include "IOService.hpp"
#include "NetCal.hpp"

// ./TcpEchoSever port
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        LOG(INFO, "USAGE METHOD -> %s PORT", argv[0]);
        exit(-1);
    }

    uint16_t port = std::stoi(argv[1]);

    Calculator cal;
    business_task process_func = std::bind(&Calculator::Calculate, &cal, std::placeholders::_1);

    IOService ios(process_func);
    task_t io_task = std::bind(&IOService::IOExcute, &ios, std::placeholders::_1);

    TcpEchoServer server(port, io_task);
    server.Start();

    return 0;
}