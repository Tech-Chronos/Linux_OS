#include "Log.hpp"
#include "InetAddr.hpp"

#include <iostream>
#include <string>
#include <memory>

#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ./TcpEchoClient IP PORT
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        LOG(INFO, "USAGE METHOD -> %s IP PORT", argv[0]);
        exit(-1);
    }

    // 1. 获取服务器IP和port
    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);

    // 2. 打开socket文件描述符
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOG(FATAL, "socket error!");
        exit(-1);
    }
    LOG(INFO, "socket successfully, sockfd = %d", sockfd);

    // 3. 尝试连接服务器
    sockaddr_in server;
    bzero(&server, sizeof server);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

    while (true)
    {
        int ret = connect(sockfd, (sockaddr *)&server, sizeof server);
        if (ret < 0)
        {
            LOG(FATAL, "connect error!");
            break;
        }
        else
        {
            LOG(INFO, "connect successfully!");
            while (true)
            {
                std::string message;
                std::cout << "Please input message# ";
                std::getline(std::cin, message);
                int n = write(sockfd, message.c_str(), message.size());

                if (n < 0)
                {
                    LOG(FATAL, "write error!");
                    break;
                }
                else if (n > 0)
                {
                    LOG(INFO, "write successfully!");
                    char buffer[1024];
                    n = read(sockfd, buffer, sizeof(buffer));
                    if (n < 0)
                    {
                        LOG(FATAL, "read error!");
                        break;
                    }
                    else if (n == 0)
                    {
                        LOG(FATAL, "server close!");
                        break;
                    }
                    else
                    {
                        buffer[n] = 0;
                        LOG(INFO, "%s", buffer);
                    }
                }
            }
        }
    }
    close(sockfd);
    return 0;
}