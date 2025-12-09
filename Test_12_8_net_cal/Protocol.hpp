#pragma once
#include "Log.hpp"

#include <iostream>
#include <string>
#include <memory>

#include <jsoncpp/json/json.h>

const std::string sep = "\n";

// 读到的信息可能并不完整(面向字节流),可以规定完整信息
// 规定："len\n{json}\n"是完整信息
// 给数据添加报头信息
std::string Encode(std::string &json_string)
{
    size_t len = json_string.size();

    std::string str_len = std::to_string(len);

    std::string result = str_len + sep + json_string + sep;

    return result;
}

bool Decode(std::string &packagestream, std::string *full_message)
{
    auto pos = packagestream.find(sep);
    if (pos == std::string::npos)
    {
        LOG(INFO, "incomplete header, waiting for more data");
        *full_message = std::string();
        return false;
    }
    // 1. 读取长度
    std::string len_str = packagestream.substr(0, pos);
    int json_len = std::stoi(len_str);

    // 2. 查看报文是否完整
    // size_t total_len = len_str.size() + json_len + sep.size();

    size_t header_len = pos + sep.size();
    size_t total_len = header_len + json_len + sep.size();
    if (packagestream.size() < total_len)
    {
        *full_message = std::string();
        return false;
    }

    // 3. 截取json串
    // int start_pos = pos + sep.size();
    std::string json_str = packagestream.substr(header_len, json_len);

    // 4. 将packagestream头部的信息删掉
    packagestream.erase(0, total_len);
    *full_message = json_str;
    return true;
}

class Request
{
public:
    Request()
    {
    }

    std::string Serialize()
    {
        Json::Value root;
        root["x"] = _x;
        root["y"] = _y;
        root["oper"] = _oper;

        Json::FastWriter writer;
        std::string result = writer.write(root);

        return result;
    }

    bool Deserialize(const std::string &netmessage)
    {
        std::cout << "addr=" << (void *)netmessage.data()
                  << ", size=" << netmessage.size() << std::endl;

        for (unsigned char c : netmessage)
            printf("%02X ", c);
        printf("\n");

        std::cout << "inner Deserialize" << std::endl;
        Json::Value root;
        Json::Reader reader;

        std::string msg = netmessage;
        while (msg.size() && isspace(msg.back()))
            msg.pop_back();
        bool res = reader.parse(msg, root);
        if (!res)
        {
            std::cerr << "parse error" << std::endl;
            return false;
        }

        _x = root["x"].asInt();
        _y = root["y"].asInt();
        _oper = root["oper"].asInt();

        return true;
    }

    ~Request()
    {
    }

    int GetX()
    {
        return _x;
    }

    int GetY()
    {
        return _y;
    }

    char GetOper()
    {
        return _oper;
    }

public:
    int _x;
    int _y;
    int _oper;
};

class Response
{
public:
    Response()
    {
    }

    std::string Serialize()
    {
        Json::Value root;
        root["result"] = _result;
        root["exit_code"] = _exit_code;
        root["desc"] = _desc;

        Json::FastWriter writer;

        std::string result = writer.write(root);
        return result;
    }

    bool Deserialize(const std::string &netmessage)
    {
        Json::Value root;
        Json::Reader reader;

        std::string msg = netmessage;
        while (msg.size() && isspace(msg.back()))
            msg.pop_back();

        if (!reader.parse(msg, root))
        {
            std::cerr << "parse error" << std::endl;
            return false;
        }

        _result = root["result"].asInt();
        _exit_code = root["exit_code"].asInt();
        _desc = root["desc"].asString();

        return true;
    }

    ~Response()
    {
    }

    void Print()
    {
        std::cout << "_result = " << _result << ", _exit_code = " << _exit_code << ", _desc = " << _desc << std::endl;
    }

public:
    int _result;       // 结果
    int _exit_code;    // 退出码
    std::string _desc; // 退出信息
};

class Factory
{
public:
    static std::shared_ptr<Request> BuildRequest()
    {
        return std::make_shared<Request>();
    }

    static std::shared_ptr<Response> BuildResponse()
    {
        return std::make_shared<Response>();
    }
};