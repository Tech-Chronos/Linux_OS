#pragma once
#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class InAddr
{
private:
    void ToHost()
    {
        _port = ntohs(_net_sock.sin_port);
        char buffer[128];
        inet_ntop(AF_INET, &_net_sock.sin_addr, buffer, sizeof(buffer));

        _ip = buffer;
    }
public:

    InAddr()
    {}
    
    InAddr(sockaddr_in sock)
        :_net_sock(sock)
    {
        ToHost();
    }

    uint16_t GetHostPort()
    {
        return _port;
    }

    std::string GetHostIp()
    {
        return _ip;
    }

    std::string Addr_Str()
    {
        return _ip + ": " + std::to_string(_port);
    }

    ~InAddr()
    {}
private:
    uint16_t _port;
    std::string _ip;
    sockaddr_in _net_sock;
};