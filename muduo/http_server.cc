#include "http/httpserver.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2 && argc != 3)
    {
        printf("Usage: %s <port> [base_dir]\n", argv[0]);
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
    std::string base_dir = argc == 3 ? argv[2] : "/home/dsj/muduo/http/wwwroot";

    HttpServer server(port, base_dir);
    server.SetThreadCountAndInit(4);
    server.EnableInactiveRelease(10);

    server.Get("/ping", [](const HttpRequest&, HttpResponse* resp) {
        resp->SetContent("pong", "text/plain; charset=utf-8");
    });

    server.Start();
    return 0;
}
