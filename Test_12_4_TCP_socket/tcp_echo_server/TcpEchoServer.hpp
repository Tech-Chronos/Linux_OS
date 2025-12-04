#pragma once
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

int gbacklog = 8;

class TcpServer
{
public:
    TcpServer(uint16_t port)
        : _port(port), _isrunning(false)
    {
    }

    void Init()
    {
        // 1. 开启 socket
        _listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_listen_sockfd < 0)
        {
            LOG(FATAL, "socket errror!");
            exit(-1);
        }
        else
        {
            LOG(INFO, "socket successfully!");
            sockaddr_in sock;
            memset(&sock, 0, sizeof(sock));
            sock.sin_family = AF_INET;
            sock.sin_port = htons(_port);

            // 2. 绑定任意IP地址和指定PORT
            // inet_pton(AF_INET, INADDR_ANY, &sock.sin_addr);
            sock.sin_addr.s_addr = INADDR_ANY;
            int ret = bind(_listen_sockfd, (sockaddr *)&sock, sizeof(sock));
            if (ret < 0)
            {
                LOG(FATAL, "bind error!");
                exit(-1);
            }
            else
            {
                LOG(INFO, "bind successfully");
                // 3. listen
                ret = listen(_listen_sockfd, gbacklog);
                if (ret < 0)
                {
                    LOG(WARNING, "listen error!");
                }
                else
                {
                    LOG(INFO, "listen successfully!");
                }
            }
        }
    }

    void Start()
    {
        _isrunning = true;
        while (true)
        {
            sockaddr_in addr;
            socklen_t len = sizeof(addr);
            memset(&addr, 0, len);
            int service_fd = accept(_listen_sockfd, (sockaddr *)&addr, &len);
            if (service_fd < 0)
            {
                LOG(WARNING, "accept error!");
            }
            else
            {
                service(service_fd, addr);
            }
        }
    }

    void service(int service_fd, sockaddr_in addr)
    {
        while (true)
        {
            LOG(INFO, "accept successfully!");
            char buffer[1024];
            InAddr peer(addr);
            int n = read(service_fd, buffer, sizeof(buffer) - 1);

            if (n < 0)
            {
                LOG(FATAL, "read error!");
                break;
            }
            else if (n == 0)
            {
                LOG(ERROR, "client: %s quit", peer.Addr_Str().c_str());
                break;
            }
            else
            {
                buffer[n] = 0;
                LOG(DEBUG, "[%s:%d]# %s", peer.GetHostIp().c_str(), peer.GetHostIp(), buffer);

                std::string echo_string = "[server echo]# ";
                echo_string += buffer;
                write(service_fd, echo_string.c_str(), echo_string.size());
            }
        }
    }
    ~TcpServer()
    {
    }

private:
    int _listen_sockfd;
    uint16_t _port;
    bool _isrunning;
};