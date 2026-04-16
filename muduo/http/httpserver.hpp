#include "../server.hpp"
#include <fstream>
#include <unordered_map>
#include <sys/stat.h>


std::unordered_map<int, std::string> kHttpStatusText =
{
    // 1xx
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},
    {103, "Early Hints"},

    // 2xx
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},

    // 3xx
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},

    // 4xx
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a Teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},

    // 5xx
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}
};


std::unordered_map<std::string, std::string> kMimeTypes = 
{
    // text
    {".html", "text/html"},
    {".htm",  "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".mjs",  "application/javascript"},
    {".txt",  "text/plain"},
    {".csv",  "text/csv"},
    {".xml",  "application/xml"},
    {".json", "application/json"},
    {".md",   "text/markdown"},

    // images
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif",  "image/gif"},
    {".bmp",  "image/bmp"},
    {".webp", "image/webp"},
    {".svg",  "image/svg+xml"},
    {".ico",  "image/x-icon"},

    // fonts
    {".woff",  "font/woff"},
    {".woff2", "font/woff2"},
    {".ttf",   "font/ttf"},
    {".otf",   "font/otf"},

    // audio
    {".mp3",  "audio/mpeg"},
    {".wav",  "audio/wav"},
    {".ogg",  "audio/ogg"},
    {".flac", "audio/flac"},
    {".aac",  "audio/aac"},

    // video
    {".mp4",  "video/mp4"},
    {".webm", "video/webm"},
    {".avi",  "video/x-msvideo"},
    {".mov",  "video/quicktime"},
    {".mkv",  "video/x-matroska"},

    // archives
    {".zip",  "application/zip"},
    {".tar",  "application/x-tar"},
    {".gz",   "application/gzip"},
    {".7z",   "application/x-7z-compressed"},
    {".rar",  "application/vnd.rar"},

    // binary / misc
    {".pdf",  "application/pdf"},
    {".wasm", "application/wasm"},
    {".bin",  "application/octet-stream"},
    {".exe",  "application/octet-stream"}
};

// // // // // // // // // // // // Util 模块 // // // // // // // // // // // // // // // 
class Util
{
public:
    // "abc,b,cvbf,fda"
    static int Split(const std::string& src, const std::string& sep, std::vector<std::string>* arr)
    {
        // 如果是空，直接返回
        if (src.empty())
            return 0;
        int offset = 0;
        while (offset < src.size())
        {
            int pos = src.find(sep, offset);

            if (pos != std::string::npos)
            {
                arr->push_back(src.substr(offset, pos - offset));
                offset = pos + sep.size();
            }
            else
            {
                arr->push_back(src.substr(offset));
                break;
            }
        }
        return arr->size();
    }


    static bool ReadFile(const std::string& filename, std::string* buffer)
    {
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs)
        {
            ERR_LOG("OPEN FILE %s ERROR!", filename.c_str());
            return false;
        }
        ifs.seekg(0, ifs.end);
        int filesize = ifs.tellg();
        ifs.seekg(0, ifs.beg);

        buffer->resize(filesize);

        ifs.read(buffer->data(), filesize);
        if (!ifs)
        {
            ERR_LOG("READ FILE %s ERROR", filename.c_str());
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }

    static bool WriteFile(const std::string& filename, const std::string& buffer)
    {
        std::ofstream ofs(filename);
        if (!ofs)
        {
            ERR_LOG("OPEN FILE %s ERROR!", filename.c_str());
            return false;
        }

        ofs.write(buffer.data(), buffer.size());
        if (!ofs.good())
        {
            ERR_LOG("WRITE FILE %s ERROR!", filename.c_str());
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }

    static std::string UrlEncode(const std::string& url, bool space_to_plus)
    {
        std::string ret;

        for (auto ch : url)
        {
            if (ch == '.' || ch == '-' || ch == '_' || ch == '~' || isalnum(ch))
            {
                ret += ch;
            }
            else if (ch == ' ' && space_to_plus)
            {
                ret += '+';
            }
            else
            {
                char tmp[4] = { 0 };
                snprintf(tmp, 4, "%%%02X", ch);
                ret += tmp;
            }
        }

        return ret;
    }

    static char HexToDecimal(char hex)
    {
        if (hex >= '0' && hex <= '9')
        {
            return hex - '0';
        }
        else if (hex >= 'A' && hex <= 'F')
        {
            return hex - 'A' + 10;
        }
        else if (hex >= 'a' && hex <= 'f')
        {
            return hex - 'a' + 10;
        }
        return -1;
    }

    static std::string UrlDecode(const std::string& url, bool plus_to_space)
    {
        std::string ret;

        for (int i = 0; i < url.size(); ++i)
        {
            if (url[i] == '%' || i + 2 < url.size())
            {
                char left = url[i + 1];
                char right = url[i + 2];

                if (left >= 0 && right >= 0)
                {
                    char character = (left << 4) + right;
                    ret += character;
                    i += 2;
                }
            }
            else if (url[i] == '+' && plus_to_space)
            {
                ret += ' ';
            }
            else 
            {
                ret += url[i];
            }
        }
        return ret;
    }

    std::string Util::GetStatusDesc(int code)
    {
        auto it = kHttpStatusText.find(code);
        if (it == kHttpStatusText.end()) return "Unknown Code";
        return it->second;
    }

    std::string Util::GetMime(const std::string& filename)
    {
        size_t pos = filename.rfind(".");
        if (pos == std::string::npos) return "Unkwon Suffix";

        std::string suffix = filename.substr(pos);

        auto it = kMimeTypes.find(suffix);
        if (it == kMimeTypes.end()) return "Unkwon Suffix";
        else return it->second;
    }

    bool IsDirectory(const std::string& filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);

        if (ret < 0) return false;
        return S_ISDIR(st.st_mode);
    }

    // 判断是否是普通文件
    bool IsRegular(const std::string& filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);

        if (ret < 0) return false;
        return S_ISREG(st.st_mode); // mode 是文件类型 + 权限
    }

    bool IsValidPath(const std::string& path)
    {
        int level = 0;
        std::vector<std::string> subdir;
        Split(path, "/", &subdir);
        for (auto& e : subdir)
        {
            if (e == "..")
            {
                --level;
                if (level < 0) return false;
            }
            else if (e == ".")
            {
                continue;
            }
            else
            {
                ++level;
            }
        }
        return true;
    }
};

// // // // // // // // // // // // HttpRequest // // // // // // // // // // // // // // // 
class HttpRequest
{
public: 
    void SetHeaders(const std::string& key, const std::string& val)
    {
        _req_headers[key] = val;
    }

    bool HasHeaders(const std::string& key)
    {
        auto it = _req_headers.find(key);
        if (it == _req_headers.end())
            return false;
        return true;
    }

    std::string GetHeaders(const std::string& key)
    {
        if (HasHeaders(key))
            return _req_headers[key];
        return "";
    }

    void SetParams(const std::string& key, const std::string& val)
    {
        _params[key] = val;
    }

    bool HasParams(const std::string& key)
    {
        auto it = _params.find(key);
        if (it == _params.end()) return false;
        return true;
    }

    std::string GetParams(const std::string& key)
    {
        if (HasParams(key)) 
            return _params[key];
        return "";
    }

    int ContentLength()
    {
        if (HasHeaders("Content-Length"))
        {
            std::string ret = GetHeaders("Content-Length");
            return std::stoi(ret);
        }
        return 0;
    }

    bool IsKeepAlive()
    {
        if (HasHeaders("Connection") && GetHeaders("Connection") == "keep-alive")
            return true;
        return false;
    }

    void HttpRequest::Reset()
    {
        _method.clear();
        _http_version.clear();
        _uri.clear();
        _req_headers.clear();
        _params.clear();
    }

public:
    std::string _method; // 请求方法
    std::string _uri; //请求的path + 查询字符串
    std::string _path; // 请求资源路径
    std::string _query_string; // 查询字符串
    std::string _http_version; // http 版本

    std::string _body; // 请求正文
    std::unordered_map<std::string, std::string> _params;
    std::unordered_map<std::string, std::string> _req_headers;
};

// // // // // // // // // // // // HttpResponse // // // // // // // // // // // // // // // 
class HttpResponse
{
public:
    HttpResponse(int code)
        : _code(code)
        , _redirect_flag(false)
    {}

    void SetHeaders(const std::string& key, const std::string& val)
    {
        _headers.insert(std::make_pair(key, val));
    }

    bool HasHeaders(const std::string& key) const
    {
        auto it = _headers.find(key);
        if (it == _headers.end()) return false;
        return true;
    }

    std::string GetHeaders(const std::string& key)
    {
        if (HasHeaders(key))
        {
            return _headers[key];
        }
        return "";
    }

    void SetRedirect(const std::string &redirect_uri, int code)
    {
        _code = code;
        _redirect_flag = true;
        _redirect_uri = redirect_uri;
    }

    void SetContent(const std::string& body, const std::string& type)
    {
        _body = body;
        SetHeaders("Content-Type", type);
    }

    bool IsKeepAlive()
    {
        if (HasHeaders("Connection") && GetHeaders("Connection") == "keep-alive")
            return true;
        return false;
    }

    void Reset()
    {
        _code = 200;
        _headers.clear();
        _body.clear();
        _redirect_flag = false;
        _redirect_uri.clear();
    }

public:
    int _code;

    std::unordered_map<std::string, std::string> _headers;
    std::string _body;
    bool _redirect_flag;
    std::string _redirect_uri;
};

// // // // // // // // // // // // HttpContext // // // // // // // // // // // // // // // 
const static int MaxLine = 8192;
typedef enum
{
    RECV_REQ_LINE,
    RECV_REQ_HEAD,
    RECV_REQ_BODY,
    RECV_REQ_OVER,
    RECV_REQ_ERROR
} HttpRecvStatus;

class HttpContext
{
private:
    bool RecvReqLine(Buffer *buf)
    {
        if (_status != RECV_REQ_LINE)
            return false;
        // 没有找到 \r\n 一整行数据
        std::string req_line = buf->GetLineAndPop();
        if (req_line.empty())
        {
            if (buf->ReadableSize() > MaxLine)
            {
                _resp_code = 414;
                _status = RECV_REQ_ERROR;
                return false;
            }
            return true;
        }
        // 有一整行数据，但是太长了
        if (req_line.size() > MaxLine)
        {
            _resp_code = 414;
            _status = RECV_REQ_ERROR;
            return false;
        }
        bool ret = ParseReqLine(req_line);
        if (ret == false)
            return false;

        _status = RECV_REQ_HEAD;
        return true;
    }

    bool ParseReqLine(const std::string& req_line)
    {
        // GET /index?a=20 HTTP/1.1
        size_t pos1 = req_line.find(' ');
        if (pos1 == std::string::npos) return false;

        size_t pos2 = req_line.find(' ', pos1 + 1);
        if (pos2 == std::string::npos) return false;

        _request._method = req_line.substr(0, pos1);
        _request._uri = req_line.substr(pos1 + 1, pos2 - pos1 - 1);
        _request._http_version = req_line.substr(pos2 + 1);

        if (_request._method.empty() || _request._uri.empty() || _request._http_version.empty())
            return false;

        size_t qpos = _request._uri.find('?');
        if (qpos == std::string::npos)
        {
            _request._path = _request._uri;
            _request._query_string.clear();
            return true;
        }
    }

public:
    HttpContext::HttpContext()
        : _resp_code(200), _status(RECV_REQ_LINE)
    {
    }


private:
    int _resp_code;
    HttpRecvStatus _status;
    HttpRequest _request;
};