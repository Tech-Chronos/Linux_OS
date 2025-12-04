#include "InetAddr.hpp"
#include "Log.hpp"

#include <iostream>
#include <string>

#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        LOG(INFO, "USAGE METHOD -> %s IP PORT", argv[0]);
        exit(-1);
    }

    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);

    // 1. 打开网卡
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOG(FATAL, "socket error!");
        exit(-1);
    }
    sockaddr_in server;
    socklen_t len = sizeof(server);
    memset(&server, 0, len);

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

    int n = connect(sockfd, (sockaddr *)&server, len);
    if (n < 0)
    {
        LOG(ERROR, "connect error!");
    }
    else
    {
        // LOG(INFO, "connect successfully!");

        while (true)
        {
            std::string message;
            std::cout << "Please input message:";
            std::getline(std::cin, message);

            int n = write(sockfd, message.c_str(), message.size());
            if (n < 0)
            {
                LOG(ERROR, "write error!");
                break;
            }
            else
            {
                LOG(INFO, "write successfully!");
                char buffer[1024];
                n = read(sockfd, buffer, sizeof(buffer));
                if (n > 0)
                {
                    buffer[n] = 0;
                    LOG(INFO, "%s", buffer);
                }
                else
                {
                    LOG(ERROR, "read error!");
                }
            }
        }
    }
    close(sockfd);

    return 0;
}
