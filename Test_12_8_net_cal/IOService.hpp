#pragma once
#include "InetAddr.hpp"
#include "Socket.hpp"
#include "Protocol.hpp"

#include <iostream>
#include <functional>

using business_task = std::function<std::shared_ptr<Response>(std::shared_ptr<Request>)>;

class IOService
{
public:
    IOService(business_task process_func)
        :_process_func(process_func)
    {
    }

    void IOExcute(SockPtr sock)
    {
        while (true)
        {
            // 1. 接收完整一条消息
            char buffer[1024];
            sock->RecvMessage(buffer);

            std::string full_message;
            std::string package_string = buffer;
            if (!Decode(package_string, &full_message))
            {
                exit(-1);
            }
            // 2. 将消息进行反序列化
            auto req_ptr = Factory::BuildRequest();
            req_ptr->Deserialize(full_message);

            // 3. 处理请求
            auto resp = _process_func(req_ptr);

            // 4. 将响应序列化
            std::string echo_message = resp->Serialize();
            
            // 5. 给请求加上报头
            echo_message = Encode(echo_message);

            // 6. 发送数据给客户端
            sock->SendMessage(echo_message);
        }
    }

    ~IOService()
    {
    }

private:
    business_task _process_func;
};