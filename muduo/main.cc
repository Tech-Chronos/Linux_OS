#include "server.hpp"

int main()
{
    Buffer buf;
    Socket sock;
    TimerWheel timer;
    EventLoop loop;
    Any any;
    Connection conn(1,1,0, &loop);
    return 0;
}