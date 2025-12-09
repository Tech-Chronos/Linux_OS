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
        : _process_func(process_func)
    {
    }

    void IOExcute(SockPtr sock)
    {
        std::string package_string;
        while (true)
        {
            // 1. 接收完整一条消息
            std::string in;
            sock->RecvMessage(&in);

            // std::cout << "server receive: " << in << std::endl;

            std::string full_message;
            package_string += in;
            if (!Decode(package_string, &full_message))
            {
                continue;
            }

            std::cout << "server decode: " << full_message << std::endl;

            // std::cout << "raw data hex:" << std::endl;
            // for (unsigned char c : full_message)
            // {
            //     printf("%02X ", c);
            // }
            // printf("\n");

            // 2. 将消息进行反序列化
            auto req_ptr = Factory::BuildRequest();

            std::cout << "server begin to Deserialize ..." << std::endl;
            bool ret = req_ptr->Deserialize(full_message);

            if (!ret)
            {
                std::cerr << "Deserialize error" << std::endl;
                exit(-1);
            }
            // 3. 处理请求
            auto resp = _process_func(req_ptr);

            std::cout << "resp->_result :" << resp->_result << ", resp->_exit_code : " << resp->_exit_code << ", resp->_desc: " << resp->_desc << std::endl;

            // 4. 将响应序列化
            std::string echo_message = resp->Serialize();

            // 5. 给请求加上报头
            echo_message = Encode(echo_message);

            std::cout << "server echo: " << echo_message << std::endl;

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