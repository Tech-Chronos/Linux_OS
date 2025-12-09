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
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

int g_backlog = 8;

class BaseSocket;

using SockPtr = std::shared_ptr<BaseSocket>;

class BaseSocket
{
public:
    virtual void CreateSocket() = 0;
    virtual void ServerBind(uint16_t port) = 0;
    virtual void ServerListen() = 0;
    virtual SockPtr ServerAccept(InAddr *addr) = 0;
    virtual bool ClientConnect(uint16_t port, std::string &ip) = 0;

    virtual int RecvMessage(std::string* in) = 0;
    virtual void SendMessage(const std::string &message) = 0;

    virtual int GetSockfd() = 0;
    virtual void Closefd() = 0;

    virtual ~BaseSocket() {}

public:
    void StartTcpServer(uint16_t port)
    {
        CreateSocket();
        ServerBind(port);
        ServerListen();
    }

    bool StartTcpClient(uint16_t port, std::string ip)
    {
        CreateSocket();
        return ClientConnect(port, ip);
    }
};

class TcpSocket : public BaseSocket
{
public:
    TcpSocket() = default;

    TcpSocket(int sockfd)
        : _sockfd(sockfd)
    {
    }

    void CreateSocket() override
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0)
        {
            LOG(FATAL, "socket error!");
            exit(-1);
        }
        LOG(INFO, "socket successfully, sockfd = %d", _sockfd);
    }

    void ServerBind(uint16_t port) override
    {
        sockaddr_in local;
        memset(&local, 0, sizeof local);
        local.sin_family = AF_INET;
        local.sin_port = htons(port);
        local.sin_addr.s_addr = INADDR_ANY;

        int ret = bind(_sockfd, (sockaddr *)&local, sizeof(local));
        if (ret < 0)
        {
            LOG(FATAL, "bind error!");
            exit(-1);
        }
        else
        {
            LOG(INFO, "bind successfully!");
        }
    }

    void ServerListen() override
    {
        int ret = listen(_sockfd, g_backlog);
        if (ret < 0)
        {
            LOG(ERROR, "listen error!");
        }
        else
        {
            LOG(INFO, "I am listening!");
        }
    }

    SockPtr ServerAccept(InAddr *addr) override
    {
        std::cout << "main thread begin to accept ..." << std::endl;
        sockaddr_in peer;
        memset(&peer, 0, sizeof(peer));
        socklen_t len = sizeof(peer);
        int service_fd = accept(_sockfd, (sockaddr *)&peer, &len);

        if (service_fd < 0)
        {
            LOG(FATAL, "accept error!");
            return nullptr;
        }
        else
        {
            InAddr client(peer);
            *addr = client;
            LOG(INFO, "connect %s successfully! service fd = %d", addr->Addr_Str().c_str(), service_fd);
        }
        return std::make_shared<TcpSocket>(service_fd);
    }

    bool ClientConnect(uint16_t port, std::string &ip)
    {
        sockaddr_in server;
        bzero(&server, sizeof server);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

        int ret = connect(_sockfd, (sockaddr *)&server, sizeof(server));
        if (ret < 0)
        {
            LOG(FATAL, "connect error!");
            return false;
        }
        else
        {
            LOG(INFO, "connect successfully!");
            return true;
        }
    }

    int GetSockfd() override
    {
        return _sockfd;
    }

    int RecvMessage(std::string* in) override
    {
        char inbuffer[4096];
        int n = recv(_sockfd, inbuffer, sizeof(inbuffer) - 1, 0);
        
        if (n > 0)
        {
            inbuffer[n] = 0;
            *in = inbuffer;
        }
        else
        {
            LOG(ERROR, "recv error or server quit!");
            exit(-1);
        }
        std::cout << *in << std::endl;
        return n;
    }

    void SendMessage(const std::string &message) override
    {
        send(_sockfd, message.c_str(), message.size(), 0);
    }

    void Closefd()
    {
        if (_sockfd > 0)
        {
            close(_sockfd);
        }
    }
    ~TcpSocket()
    {
    }

private:
    int _sockfd;
};