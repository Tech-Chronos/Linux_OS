#pragma once
#include "Log.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <memory>

const static std::string base_sep = "\n";
const static std::string line_sep = ": ";

class HttpRequest
{
private:
    // 解析请求行
    void ParseLine(std::string &netmessage)
    {
        auto pos = netmessage.find(base_sep);
        if (pos == std::string::npos)
        {
            LOG(INFO, "find error!");
            return;
        }
        _req_line = netmessage.substr(0, pos);

        ParseDetailLine();
        netmessage.erase(0, pos + base_sep.size());
    }

    // 解析报头和数据
    void ParseHeaderAndData(std::string &netmessage)
    {
        while (true)
        {
            auto pos = netmessage.find(base_sep);
            if (pos == std::string::npos)
            {
                LOG(INFO, "find error!");
                break;
            }
            std::string header = netmessage.substr(0, pos);
            if (header.empty())
            {
                netmessage.erase(0, base_sep.size());
                _data = netmessage.substr(0);
                break;
            }
            else
            {
                _req_header.push_back(header);
                netmessage.erase(0, pos + base_sep.size());
            }
        }
        ParseHeaderKV();
    }

    void ParseDetailLine()
    {
        std::stringstream ss(_req_line);
        ss >> _req_func >> _url >> _http_version;
    }

    void ParseHeaderKV()
    {
        for (auto &header : _req_header)
        {
            auto pos = header.find(line_sep);

            if (pos == std::string::npos)
            {
                LOG(ERROR, "find error");
                break;
            }
            else
            {
                std::string key = header.substr(0, pos);
                std::string value = header.substr(pos + line_sep.size());

                _headers_kv.insert({key, value});
            }
        }
    }

public:
    HttpRequest()
    {
    }
    void Serialize()
    {
    }

    void Deseralize(std::string &netmessage)
    {
        ParseLine(netmessage);

        ParseHeaderAndData(netmessage);
    }

    void Print()
    {
        std::cout << "======== HTTP Request ========\n";
        std::cout << "Request Line: " << _req_line << "\n";
        std::cout << "Method      : " << _req_func << "\n";
        std::cout << "URL         : " << _url << "\n";
        std::cout << "Version     : " << _http_version << "\n\n";

        std::cout << "Headers:\n";
        for (auto &kv : _headers_kv)
        {
            std::cout << "  " << kv.first << " = " << kv.second << "\n";
        }

        std::cout << "\nBody Data:\n"
                  << _data << "\n";
        std::cout << "================================\n";
    }

    ~HttpRequest()
    {
    }

private:
    std::string _req_line;                // 请求行
    std::vector<std::string> _req_header; // 请求报头
    std::string _data;                    // 有效载荷

    std::string _req_func;
    std::string _url;
    std::string _http_version;

    std::unordered_map<std::string, std::string> _headers_kv;
};

class HttpServer
{
public:
    HttpServer()
    {
    }

    std::string HandlerRequest(std::string &req)
    {
#ifdef TEST
        std::cout << "request -> " << req << std::endl;

        return std::string();
#else

        std::unique_ptr<HttpRequest> usptr = std::make_unique<HttpRequest>();
        std::cout << "start to deseralize ..." << std::endl;
        usptr->Deseralize(req);

        usptr->Print();

        return std::string();
    }
    ~HttpServer()
    {
    }

private:
};

#endif