#include "Log.hpp"
#include "InetAddr.hpp"
#include "IOService.hpp"
#include "Socket.hpp"

#include <iostream>
#include <string>
#include <memory>

#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ./TcpEchoClient IP PORT
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        LOG(INFO, "USAGE METHOD -> %s IP PORT", argv[0]);
        exit(-1);
    }

    // 1. 获取服务器IP和port
    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);

    // 2. 打开socket文件描述符
    std::shared_ptr<TcpSocket> client = std::make_shared<TcpSocket>();

    srand(time(nullptr) ^ getpid());
    std::string oper_str = "+-*/%&";

    std::string recvmessage;

    if (client->StartTcpClient(port, ip))
    {
        while (true)
        {
            // 1. 构建请求
            auto req = Factory::BuildRequest();
            req->_x = rand() % 10;
            req->_y = rand() % 10;
            req->_oper = oper_str[req->_y % oper_str.size()];

            // 2. 进行序列化
            std::string req_str = req->Serialize();
            req_str = Encode(req_str);

            std::cout << "client req_str: " << req_str << std::endl;

            // 3. 发送给服务器
            client->SendMessage(req_str);

            // 4. 接收应答
            std::string in;
            client->RecvMessage(&in);
            recvmessage += in;

            std::cout << "recvmessage ->" << recvmessage << std::endl;
            // 5. decode
            std::string full_message;
            while (Decode(recvmessage, &full_message))
            {
                auto resp = Factory::BuildResponse();
                if (resp->Deserialize(full_message))
                {
                    resp->Print();
                }
                else
                {
                    std::cerr << "Deserialize error!" << std::endl;
                }
            }
            sleep(100);
        }
    }

    client->Closefd();
    return 0;
}