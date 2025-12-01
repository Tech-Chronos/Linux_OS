#include "Log.hpp"

#include <iostream>
#include <string>
#include <memory>

#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Log lg;

uint16_t gport = 8888;
std::string gip = "127.0.0.1";

class UdpServer
{
public:
    UdpServer(uint16_t port = gport, std::string ip = gip)
        : _port(port), _ip(ip)
    {
    }

    void Init()
    {
        // 1. socket,可以理解为打开网卡
        _socketfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_socketfd < 0)
        {
            LOG(FATAL, "socket error!");
            exit(-1);
        }
        else
        {
            LOG(DEBUG, "socket fd = %d", _socketfd);
        }

        // 2. socket绑定 IP & PORT
        struct sockaddr_in addr;
        // 2.1 先清空
        bzero(&addr, sizeof(addr));

        // 2.2 设置 IPV4
        addr.sin_family = AF_INET;

        // 2.3 设置端口
        // 网络传输默认是需要大端的字节序
        // 将主机字节序转换成网络字节序
        // (_port << 8) | (_port >> 8)即可;
        addr.sin_port = htons(_port);

        // 2.4 设置 IP 地址
        // 将ip从点分十进制的字符串形式转换成一个四字节的整数
        // 将这个整数转换成网络所需要的大端的字节序
        addr.sin_addr.s_addr = inet_addr(_ip.c_str());

        // 3. 绑定到socket
        int ret = bind(_socketfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
        if (ret == 0)
        {
            LOG(DEBUG, "bind success");
        }
        else
        {
            LOG(FATAL, "bind failture");
            exit(-1);
        }
    }

    void Start()
    {
        while (true)
        {
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            struct sockaddr_in peer_addr;
            socklen_t peer_len;
            int n = recvfrom(_socketfd, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr *>(&peer_addr), &peer_len);

            buffer[n] = '\0';

            LOG(DEBUG, "recv: %s from %s:%d\n",
                   buffer,
                   inet_ntoa(peer_addr.sin_addr),
                   ntohs(peer_addr.sin_port));
        }
    }

    ~UdpServer()
    {
    }

private:
    int _socketfd;
    uint16_t _port;
    std::string _ip;
};