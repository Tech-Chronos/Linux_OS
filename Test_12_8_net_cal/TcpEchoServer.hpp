#pragma once
#include "Log.hpp"
#include "InetAddr.hpp"
#include "Socket.hpp"

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

//int g_backlog = 8;

using task_t = std::function<void(int, InAddr)>;

class TcpEchoServer;

struct ThreadData
{
    ThreadData(TcpEchoServer *self, int service_fd, InAddr addr)
        : _self(self), _service_fd(service_fd), _addr(addr)
    {
    }

    TcpEchoServer *_self;
    int _service_fd;
    InAddr _addr;
};

class TcpEchoServer
{
private:
    // 线程版本
    static void *ThreadFunc(void *args)
    {
        ThreadData *td = static_cast<ThreadData *>(args);

        pthread_detach(pthread_self());
        TcpEchoServer *server = td->_self;
        int servicefd = td->_service_fd;
        InAddr addr = td->_addr;

        server->_command_task(servicefd, addr);
        close(td->_service_fd);
        return nullptr;
    }

public:
    TcpEchoServer(uint16_t port, task_t command_task)
        : _port(port), _isrunning(false), _command_task(command_task)
    {
        _sock = std::make_shared<TcpSocket>();
    }

    void Init()
    {
        // 1. 创建监听套接字
        _sock->CreateSocket();
        // 2. 绑定 服务器的 IP 和 PORT
        _sock->ServerBind(_port);

        // 3. 监听
        _sock->ServerListen();
    }

    void Start()
    {
        // 1. 接收链接
        _isrunning = true;
        while (_isrunning)
        {
            InAddr client;
            SockPtr service = _sock->ServerAccept(&client);

            int service_fd = service->GetSockfd();

            // 创建线程去执行任务
            pthread_t tid;
            ThreadData *td = new ThreadData(this, service_fd, client);
            pthread_create(&tid, nullptr, ThreadFunc, td);
        }
    }

    ~TcpEchoServer()
    {
    }

private:
    SockPtr _sock;
    uint16_t _port;
    bool _isrunning;

    task_t _command_task;
};