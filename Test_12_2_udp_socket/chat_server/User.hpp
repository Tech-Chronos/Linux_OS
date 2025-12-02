#pragma once
#include "Log.hpp"
#include "InAddr.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include <cstring>

using func_t = std::function<void(InAddr, int, std::string)>;

class User
{
public:
    User(const InAddr& addr)
        :_addr(addr)
    {}

    InAddr GetInetAddr()
    {
        return _addr;
    }
private:
    InAddr _addr;
};

class Group
{
public:
    Group()
    {}

    void Check(User newuser)
    {
        for (auto& user : _users)
        {
            if (newuser.GetInetAddr() == user.GetInetAddr())
            {
                break;
            }
        }
        _users.push_back(newuser);
    }

    void OffLine(User olduser)
    {
        std::vector<User>::iterator it = _users.begin();
        while (it != _users.end())
        {
            if (it->GetInetAddr() == olduser.GetInetAddr())
                break;
            ++it;
        }
        _users.erase(it);
    }

    void Forward(InAddr addr, int sockfd, std::string message)
    {
        std::string ip = addr.GetIP();
        std::string port = std::to_string(addr.GetPort());

        std::string full_message = "[" + ip + ": " + port + "]# " + message; 
        for (auto& user : _users)
        {
            sockaddr_in guy;
            guy = user.GetInetAddr().Addr();

            sendto(sockfd, full_message.c_str(), full_message.size(), 0, (struct sockaddr*)&guy, sizeof(guy));
        }
    }

    void SendMessageToAll(InAddr addr, int sockfd, std::string message)
    {
        Check(addr);

        if (message.c_str() == "q")
        {
            OffLine(addr);
        }

        Forward(addr, sockfd, message);
        
    }

private:
    std::vector<User> _users;
};