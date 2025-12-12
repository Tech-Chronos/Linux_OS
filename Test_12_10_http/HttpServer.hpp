#pragma once
#include "Log.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <memory>

const static std::string base_sep = "\r\n";
const static std::string line_sep = ": ";
const static std::string http_version = "HTTP/1.0";
const static std::string net_root = "./wwwroot";
const static std::string default_page = "/index.html";
const static std::string g404_page = "/404.html";
const static std::string space = " ";

// 请求资源序反序列化
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
                _data += netmessage.substr(0);
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

        // /?a=2&b=5
        if (_url == "/")
        {
            _url = net_root + default_page;
        }
        else
        {
            // wwwroot/?a=2&b=5
            _url = net_root + _url;
        }

        if (_req_func == "GET")
        {
            // // wwwroot/?a=2&b=5
            auto pos = _url.find("?");
            if (pos != std::string::npos)
            {
                _data = _url.substr(pos + 1);
                _url.resize(pos);
            }
        }
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

    std::string GetURL()
    {
        return _url;
    }

    std::string GetData()
    {
        return _data;
    }

    std::string GetFunc()
    {
        return _req_func;
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

// 响应序列化
class HttpResponse
{
private:
    int Get404ContentAndSize(std::string &url)
    {
        url = net_root + g404_page;
        std::ifstream ifs(url, std::ios::binary);

        // 2. 获取文件大小
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        // 3. 为数据分配空间
        _data.resize(size);

        // 4. 读取内容
        ifs.read(&_data[0], size);

        return size;
    }
    int GetFileContentAndSize(std::string &url)
    {
        // 1. 打开文件
        std::ifstream ifs(url, std::ios::binary);

        if (!ifs.is_open())
        {
            _exit_code = 404;
            _exit_desc = _code_table[_exit_code];

            return Get404ContentAndSize(url);
        }
        // 2. 获取文件大小
        _exit_code = 200;
        _exit_desc = _code_table[_exit_code];

        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        // 3. 为数据分配空间
        _data.resize(size);

        // 4. 读取内容
        ifs.read(&_data[0], size);

        // std::cout << _data << std::endl;

        return size;
    }

    void SetRespLine()
    {
        _resp_line = _http_version + space + std::to_string(_exit_code) + space + _exit_desc + base_sep;
    }

    int GetSuffix(const std::string &url)
    {
        auto pos = url.rfind(".");
        if (pos == std::string::npos)
        {
            _suffix = ".default";
            LOG(FATAL, "no suffix");
            return -1;
        }

        _suffix = url.substr(pos);
        return pos;
    }

    void AddHeader()
    {
        // 插入 content type
        _header_kv["Content-Type"] = _content_type[_suffix];

        // 插入 content length
        _header_kv["Content-Length"] = std::to_string(_file_size);
    }

    void CreateFullHeaders()
    {
        for (auto &header : _header_kv)
        {
            _resp_headers += header.first + line_sep + header.second + base_sep;
        }
    }

public:
    HttpResponse()
        : _http_version(http_version), _empty_line(base_sep)
    {
        _code_table[100] = "Continue";
        _code_table[200] = "OK";
        _code_table[301] = "Moved Permanently";
        _code_table[404] = "Not Found";
        _code_table[500] = "Internal Server Error";

        _content_type[".html"] = "text/html";
        _content_type[".css"] = "text/css";
        _content_type[".js"] = "application/javascript";

        _content_type[".png"] = "image/png";
        _content_type[".jpg"] = "image/jpeg";
        _content_type[".jpeg"] = "image/jpeg";
        _content_type[".gif"] = "image/gif";
        _content_type[".ico"] = "image/x-icon";

        _content_type[".txt"] = "text/plain";
        _content_type[".json"] = "application/json";

        _content_type[".default"] = "text/html";
    }

    std::string Serialize(std::string &url)
    {

        // 判断重定向
        if (url == "./wwwroot/redir")
        {
            _exit_code = 301;
            _exit_desc = _code_table[301];

            _header_kv["Location"] = "https://www.qq.com";

            _file_size = 0;

            // LOG(DEBUG, "重定向设置成功");
        }
        else
        {
            // 1. 获取文件内容和大小 -- 退出码和退出描述的设置
            _file_size = GetFileContentAndSize(url);
        }
        // 2. 构造响应行
        SetRespLine();

        // 3. 获取url后缀，构造Content—Type
        GetSuffix(url);

        // 4. 增加报头 -- KV 结构
        AddHeader();
        // 5. 构造完整的报头 -- string
        CreateFullHeaders();

        // 6. 完整的反序列化
        _full_resp_message = _resp_line + _resp_headers + _empty_line + _data;

        LOG(DEBUG, "\n_full_resp_message = \n%s", _full_resp_message.c_str());
        return _full_resp_message;
    }

    ~HttpResponse()
    {
    }

private:
    // 完整的 response
    std::string _resp_headers; // 响应报头
    std::string _resp_line;    // 响应状态行
    std::string _empty_line;
    std::string _data; // 真正的有效载荷

    // 状态行详细信息
    std::string _http_version;
    int _exit_code;
    std::string _exit_desc;

    std::unordered_map<int, std::string> _code_table;           // 退出码和退出描述的格式
    std::unordered_map<std::string, std::string> _content_type; // 告诉客户端body的类型
    std::unordered_map<std::string, std::string> _header_kv;    // 报头 KV 结构

    int _file_size;      // 文件大小
    std::string _suffix; // 文件后缀

    std::string _full_resp_message;
};

using web_f = std::function<HttpResponse(HttpRequest &)>;
class HttpServer
{
public:
    HttpServer(web_f func)
    {
        _list_func["/login"] = func;
    }

    std::string HandlerRequest(std::string &req)
    {
#ifdef TEST
        std::cout << "request -> " << req << std::endl;

        return std::string();
#else

        std::unique_ptr<HttpRequest> req_ptr = std::make_unique<HttpRequest>();
        // std::cout << "start to deseralize ..." << std::endl;
        req_ptr->Deseralize(req);

        req_ptr->Print();

        std::string url = req_ptr->GetURL();
        // std::cout << "url -> " << url << std::endl;

        std::string req_func = req_ptr->GetFunc();
        if (req_func == "POST")
        {
            auto dot_pos = url.find(".html");
            if (dot_pos != std::string::npos)
            {}
            else
            {
                auto pos = url.rfind(net_root);

                std::string func_name = url.substr(pos + net_root.size());

                LOG(DEBUG, "func_name = %s", func_name.c_str());
                // HttpRequest req;
                HttpResponse resp = _list_func[func_name](*req_ptr);

                std::string index = "wwwroot/login.html";
                return resp.Serialize(index);
            }
        }

        std::unique_ptr<HttpResponse> resp_ptr = std::make_unique<HttpResponse>();

        return resp_ptr->Serialize(url);
#endif
    }
    ~HttpServer()
    {
    }

private:
    std::unordered_map<std::string, web_f> _list_func;
};
