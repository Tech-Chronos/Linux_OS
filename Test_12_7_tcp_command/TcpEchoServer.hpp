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

using task_t = std::function<void(int, InAddr)>;

class TcpEchoServer;

struct ThreadData
{
    ThreadData(TcpEchoServer* self, int service_fd, InAddr addr)
        :_self(self)
        ,_service_fd(service_fd)
        ,_addr(addr)
    {}

    TcpEchoServer* _self;
    int _service_fd;
    InAddr _addr;
};

class TcpEchoServer
{
private:
    // void ServiceHelper(int service_fd, InAddr addr)
    // {
    //     LOG(INFO, "%s", addr.Addr_Str().c_str());
    //     // 2. receive peer message
    //     while (true)
    //     {
    //         char buffer[1024];
    //         int n = read(service_fd, buffer, sizeof(buffer) - 1);
    //         if (n < 0)
    //         {
    //             LOG(ERROR, "read error!");
    //             break;
    //         }
    //         else if (n == 0)
    //         {
    //             LOG(ERROR, "%s quit!", addr.Addr_Str().c_str());
    //             break;
    //         }
    //         else
    //         {
    //             LOG(DEBUG, "read successfully!");
    //             buffer[n] = 0;
    //             LOG(INFO, "%s: %d say# %s", addr.GetHostIp().c_str(), addr.GetHostPort(), buffer);

    //             // 3. 服务器回复消息给客户端
    //             std::string echo_message = "server echo# ";
    //             echo_message += buffer;

    //             n = write(service_fd, echo_message.c_str(), echo_message.size());
    //             if (n < 0)
    //             {
    //                 LOG(FATAL, "write failture!");
    //                 close(service_fd);
    //                 exit(-1);
    //             }
    //             else
    //             {
    //                 LOG(INFO, "write successfully!");
    //             }
    //         }
    //     }
    // }
    
    // 线程版本
    static void* ThreadFunc(void* args)
    {
        ThreadData* td = static_cast<ThreadData*>(args);

        pthread_detach(pthread_self());
        TcpEchoServer* server = td->_self;
        int servicefd = td->_service_fd;
        InAddr addr = td->_addr;
        
        server->_command_task(servicefd, addr);
        close(td->_service_fd);
        return nullptr;
    }

public:
    TcpEchoServer(uint16_t port, task_t command_task)
        : _port(port)
        , _isrunning(false)
        , _command_task(command_task)
    {
    }

    void Init()
    {
        // 1. 创建监听套接字
        _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (_listen_sock < 0)
        {
            LOG(FATAL, "socket error!");
            exit(-1);
        }
        LOG(INFO, "socket successfully, sockfd = %d", _listen_sock);
        // 2. 绑定 服务器的 IP 和 PORT
        sockaddr_in local;
        memset(&local, 0, sizeof local);
        local.sin_family = AF_INET;
        local.sin_port = htons(_port);
        local.sin_addr.s_addr = INADDR_ANY;

        int ret = bind(_listen_sock, (sockaddr *)&local, sizeof(local));
        if (ret < 0)
        {
            LOG(FATAL, "bind error!");
            exit(-1);
        }
        else
        {
            LOG(INFO, "bind successfully!");
            // 3. 监听
            if (listen(_listen_sock, g_backlog) < 0)
            {
                LOG(ERROR, "listen error!");
            }
            else
            {
                LOG(INFO, "I am listening!");
            }
        }
    }

    void Start()
    {
        // 1. 接收链接
        _isrunning = true;
        while (_isrunning)
        {
            // 表示不需要等待子进程了
            //signal(SIGCHLD, SIG_IGN);
            sockaddr_in peer;
            memset(&peer, 0, sizeof(peer));
            socklen_t len = sizeof(peer);
            int service_fd = accept(_listen_sock, (sockaddr *)&peer, &len);
            if (service_fd < 0)
            {
                LOG(FATAL, "accept error!");
                continue;
            }
            else
            {
                InAddr addr(peer);
                LOG(INFO, "connect %s successfully! service fd = %d", addr.Addr_Str().c_str(), service_fd);

                pthread_t tid;
                
                ThreadData* td = new ThreadData(this, service_fd, addr);
                pthread_create(&tid, nullptr, ThreadFunc, td);
            }
        }
    }

    ~TcpEchoServer()
    {
        if (_isrunning)
        {
            close(_listen_sock);
        }
    }

private:
    int _listen_sock;
    uint16_t _port;
    bool _isrunning;

    task_t _command_task;
};