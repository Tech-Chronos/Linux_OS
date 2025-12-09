#pragma once
#include "Log.hpp"

#include <iostream>
#include <string>
#include <memory>

#include <jsoncpp/json/json.h>

const std::string sep = "\r\n";

// 读到的信息可能并不完整(面向字节流),可以规定完整信息
// 规定："len\r\n{json}\r\n"是完整信息
// 给数据添加报头信息
std::string Encode(std::string &json_string)
{
    size_t len = json_string.size();

    std::string str_len = std::to_string(len);

    std::string result = str_len + sep + json_string + sep;

    return result;
}

bool Decode(std::string &packagestream, std::string* full_message)
{
    int pos = packagestream.find(sep.c_str(), 0);
    if (pos == std::string::npos)
    {
        LOG(FATAL, "find error!");
        return false;
    }
    else if (pos == 0)
    {
        LOG(ERROR, "pos == 0");
        return false;
    }
    // 1. 读取长度
    std::string len_str = packagestream.substr(0, pos);
    int json_len = std::stoi(len_str);

    // 2. 截取json串
    int start_pos = pos + sep.size();
    std::string json_str = packagestream.substr(start_pos, json_len);

    // 3. 判断是否是完整的
    if (json_str.size() < json_len)
    {
        LOG(FATAL, "it isn't a full message!");
        return false;
    }

    // 4. 如果是了，将packagestream头部的信息删掉
    size_t total_len = len_str.size() + json_len + (2 * sep.size());
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

    void Deserialize(const std::string &netmessage)
    {
        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(netmessage, root))
        {
            throw std::runtime_error("JSON parse error: " + reader.getFormattedErrorMessages());
        }

        if (!root.isMember("x") || !root.isMember("y") || !root.isMember("oper"))
        {
            throw std::runtime_error("Missing field(s) in JSON");
        }

        if (!root["x"].isInt() || !root["y"].isInt() || !root["oper"].isInt())
        {
            throw std::runtime_error("Invalid JSON field type(s)");
        }

        _x = root["x"].asInt();
        _y = root["y"].asInt();
        _oper = root["oper"].asInt();
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

private:
    int _x;
    int _y;
    char _oper;
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

    void Deserialize(const std::string &netmessage)
    {
        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(netmessage, root))
        {
            throw std::runtime_error("JSON parse error: " + reader.getFormattedErrorMessages());
        }

        if (!root.isMember("result") || !root.isMember("exit_code") || !root.isMember("desc"))
        {
            throw std::runtime_error("Missing field(s) in JSON");
        }

        if (!root["result"].isInt() || !root["exit_code"].isInt() || !root["desc"].isString())
        {
            throw std::runtime_error("Invalid JSON field type(s)");
        }

        _result = root["result"].asInt();
        _exit_code = root["exit_code"].asInt();
        _desc = root["desc"].asString();
    }

    ~Response()
    {
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