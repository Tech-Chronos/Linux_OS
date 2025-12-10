#include "Socket.hpp"
#include "TcpServer.hpp"
#include "HttpServer.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        LOG(ERROR, "USAGE METHOD -> %s PORT", argv[0]);
        exit(-1);
    }

    uint16_t port = std::stoi(argv[1]);

    HttpServer http;
    TcpServer server(std::bind(&HttpServer::HandlerRequest,
                               http,
                               std::placeholders::_1),
                     port);

    server.Loop();
    return 0;
}