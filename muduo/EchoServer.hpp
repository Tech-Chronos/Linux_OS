#include "server.hpp"

class EchoServer
{
private:
    void OnConnected(const PtrConnection& conn)
    {
        INF_LOG("new connection come: %ld", conn->ConnId());
    }

    void OnClose(std::shared_ptr<Connection> conn)
    {
        INF_LOG("connection close: %ld", conn->ConnId());
    }

    void OnMessage(std::shared_ptr<Connection> conn, Buffer *buf)
    {
        if (buf->ReadableSize() > 0)
        {
            std::string data = buf->ReadStringAndPop(buf->ReadableSize());
            conn->Send(const_cast<char *>(data.data()), data.size());
        }
    }

public:
    EchoServer(uint16_t port)
        : _server(port)
    {
        _server.SetThreadCountAndInit(4);
        _server.SetMessageCallback(std::bind(&EchoServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
        _server.SetClosedCallback(std::bind(&EchoServer::OnClose, this, std::placeholders::_1));
        _server.SetConnectedCallback(std::bind(&EchoServer::OnConnected, this, std::placeholders::_1));
        _server.EnableInactiveRelease(10);
    }

    void Start()
    {
        _server.Start();
    }
private:
    TcpServer _server;
};