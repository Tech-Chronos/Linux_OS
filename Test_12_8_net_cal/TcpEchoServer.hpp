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

using task_t = std::function<void(SockPtr)>;

class TcpEchoServer;

struct ThreadData
{
    ThreadData(TcpEchoServer *self, SockPtr sock)
        : _self(self), _sock(sock)
    {
    }

    TcpEchoServer *_self;
    SockPtr _sock;
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

        server->_io_task(server->_sock);

        server->_sock->Closefd();
        return nullptr;
    }

public:
    TcpEchoServer(uint16_t port, task_t io_task)
        : _port(port), _isrunning(false), _io_task(io_task)
    {
        _sock = std::make_shared<TcpSocket>();
        _sock->StartTcpServer(_port);
    }

    void Start()
    {
        // 1. 接收链接
        _isrunning = true;
        while (_isrunning)
        {
            InAddr client;
            SockPtr service = _sock->ServerAccept(&client);

            // 创建线程去执行任务
            pthread_t tid;
            ThreadData *td = new ThreadData(this, service);
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

    task_t _io_task;
};