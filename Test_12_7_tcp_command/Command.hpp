#include "Log.hpp"
#include "InetAddr.hpp"
#include <iostream>
#include <string>

#include <cstdio>

class DealCommand
{
private:
    void ExcuteCommand(int service_fd)
    {
        FILE* file = popen(_command.c_str(), "r");
        if (!file)
        {
            LOG(FATAL, "popen error!");
            exit(-1);
        }
        else
        {
            char echo_buffer[1024];
            int n = fread(echo_buffer, sizeof(char), 1024, file);
            if (n > 0)
            {
                echo_buffer[n] = 0;
                write(service_fd, echo_buffer, sizeof(echo_buffer));
            }
            else if (n == 0)
            {
                std::string val = "command excute successfully!";
                write(service_fd, val.c_str(), val.size());
            }
            else
            {
                std::string error = "read failture!";
                write(service_fd, error.c_str(), error.size());
            }
        }
    }
public:
    DealCommand()
    {
    }

    void GetCommand(int service_fd, InAddr addr)
    {
        while (true)
        {
            char buffer[1024];
            int n = read(service_fd, buffer, sizeof(buffer) - 1);
            if (n < 0)
            {
                LOG(ERROR, "read error!");
                break;
            }
            else if (n == 0)
            {
                LOG(ERROR, "%s quit!", addr.Addr_Str().c_str());
                break;
            }
            else
            {
                LOG(DEBUG, "read successfully!");
                buffer[n] = 0;
                LOG(INFO, "%s: %d say# %s", addr.GetHostIp().c_str(), addr.GetHostPort(), buffer);

                _command = buffer;
                // 3. 服务器回复消息给客户端
                ExcuteCommand(service_fd);
            }
        }
    }

private:
    std::string _command;
};