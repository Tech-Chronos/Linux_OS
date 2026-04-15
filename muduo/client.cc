#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        return 1;
    }

    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    // 1. 创建 socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    // 2. 组织服务器地址
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }

    // 3. 发起连接
    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        return 1;
    }

    std::cout << "connected to " << ip << ":" << port << std::endl;

    // 4. 循环收发
    std::string line;
    char buffer[4096];

    while (true)
    {
        std::cout << "input> ";
        if (!std::getline(std::cin, line))
            break;

        if (line == "quit")
            break;

        // 发出去
        ssize_t n = send(sockfd, line.data(), line.size(), 0);
        if (n < 0)
        {
            perror("send");
            break;
        }

        // 读回显
        ssize_t rn = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (rn < 0)
        {
            perror("recv");
            break;
        }
        else if (rn == 0)
        {
            std::cout << "server closed connection" << std::endl;
            break;
        }

        buffer[rn] = '\0';
        std::cout << "echo from server: " << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}