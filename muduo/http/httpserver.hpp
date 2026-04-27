#pragma once

#include "../server.hpp"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

static std::unordered_map<int, std::string> kHttpStatusText = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},
    {103, "Early Hints"},
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
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
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

static std::unordered_map<std::string, std::string> kMimeTypes = {
    {".html", "text/html; charset=utf-8"},
    {".htm", "text/html; charset=utf-8"},
    {".css", "text/css; charset=utf-8"},
    {".js", "application/javascript; charset=utf-8"},
    {".mjs", "application/javascript; charset=utf-8"},
    {".txt", "text/plain; charset=utf-8"},
    {".csv", "text/csv; charset=utf-8"},
    {".xml", "application/xml; charset=utf-8"},
    {".json", "application/json; charset=utf-8"},
    {".md", "text/markdown; charset=utf-8"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".webp", "image/webp"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".ttf", "font/ttf"},
    {".otf", "font/otf"},
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {".ogg", "audio/ogg"},
    {".flac", "audio/flac"},
    {".aac", "audio/aac"},
    {".mp4", "video/mp4"},
    {".webm", "video/webm"},
    {".avi", "video/x-msvideo"},
    {".mov", "video/quicktime"},
    {".mkv", "video/x-matroska"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"},
    {".7z", "application/x-7z-compressed"},
    {".rar", "application/vnd.rar"},
    {".pdf", "application/pdf"},
    {".wasm", "application/wasm"},
    {".bin", "application/octet-stream"},
    {".exe", "application/octet-stream"}
};

class Util
{
public:
    static int Split(const std::string& src, const std::string& sep, std::vector<std::string>* arr)
    {
        if (src.empty())
            return 0;

        size_t offset = 0;
        while (offset < src.size())
        {
            size_t pos = src.find(sep, offset);
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
        return static_cast<int>(arr->size());
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
        std::streamoff filesize = ifs.tellg();
        ifs.seekg(0, ifs.beg);
        if (filesize < 0)
        {
            ERR_LOG("READ FILE %s SIZE ERROR", filename.c_str());
            return false;
        }

        buffer->resize(static_cast<size_t>(filesize));
        if (filesize == 0)
            return true;

        ifs.read(buffer->data(), filesize);
        if (!ifs)
        {
            ERR_LOG("READ FILE %s ERROR", filename.c_str());
            return false;
        }
        return true;
    }

    static bool WriteFile(const std::string& filename, const std::string& buffer)
    {
        std::ofstream ofs(filename, std::ios::binary);
        if (!ofs)
        {
            ERR_LOG("OPEN FILE %s ERROR!", filename.c_str());
            return false;
        }

        ofs.write(buffer.data(), buffer.size());
        if (!ofs.good())
        {
            ERR_LOG("WRITE FILE %s ERROR!", filename.c_str());
            return false;
        }
        return true;
    }

    static std::string UrlEncode(const std::string& url, bool space_to_plus)
    {
        std::string ret;
        for (unsigned char ch : url)
        {
            if (ch == '.' || ch == '-' || ch == '_' || ch == '~' || std::isalnum(ch))
            {
                ret += static_cast<char>(ch);
            }
            else if (ch == ' ' && space_to_plus)
            {
                ret += '+';
            }
            else
            {
                char tmp[4] = {0};
                std::snprintf(tmp, sizeof(tmp), "%%%02X", ch);
                ret += tmp;
            }
        }
        return ret;
    }

    static int HexToDecimal(char hex)
    {
        if (hex >= '0' && hex <= '9')
            return hex - '0';
        if (hex >= 'A' && hex <= 'F')
            return hex - 'A' + 10;
        if (hex >= 'a' && hex <= 'f')
            return hex - 'a' + 10;
        return -1;
    }

    static std::string UrlDecode(const std::string& url, bool plus_to_space)
    {
        std::string ret;
        for (size_t i = 0; i < url.size(); ++i)
        {
            if (url[i] == '%' && i + 2 < url.size())
            {
                int left = HexToDecimal(url[i + 1]);
                int right = HexToDecimal(url[i + 2]);
                if (left >= 0 && right >= 0)
                {
                    ret += static_cast<char>((left << 4) + right);
                    i += 2;
                    continue;
                }
            }

            if (url[i] == '+' && plus_to_space)
                ret += ' ';
            else
                ret += url[i];
        }
        return ret;
    }

    static std::string GetStatusDesc(int code)
    {
        auto it = kHttpStatusText.find(code);
        return it == kHttpStatusText.end() ? "Unknown Code" : it->second;
    }

    static std::string GetMime(const std::string& filename)
    {
        size_t pos = filename.rfind('.');
        if (pos == std::string::npos)
            return "application/octet-stream";

        auto it = kMimeTypes.find(filename.substr(pos));
        return it == kMimeTypes.end() ? "application/octet-stream" : it->second;
    }

    static bool StatPath(const std::string& filename, struct stat* st)
    {
        return stat(filename.c_str(), st) == 0;
    }

    static bool IsDirectory(const std::string& filename)
    {
        struct stat st;
        return StatPath(filename, &st) && S_ISDIR(st.st_mode);
    }

    static bool IsRegular(const std::string& filename)
    {
        struct stat st;
        return StatPath(filename, &st) && S_ISREG(st.st_mode);
    }

    static bool IsValidPath(const std::string& path)
    {
        int level = 0;
        std::vector<std::string> subdir;
        Split(path, "/", &subdir);
        for (const auto& e : subdir)
        {
            if (e.empty() || e == ".")
                continue;
            if (e == "..")
            {
                --level;
                if (level < 0)
                    return false;
            }
            else
            {
                ++level;
            }
        }
        return true;
    }
};

class HttpRequest
{
public:
    void SetHeaders(const std::string& key, const std::string& val)
    {
        _req_headers[key] = val;
    }

    bool HasHeaders(const std::string& key) const
    {
        return _req_headers.find(key) != _req_headers.end();
    }

    std::string GetHeaders(const std::string& key) const
    {
        auto it = _req_headers.find(key);
        return it == _req_headers.end() ? "" : it->second;
    }

    void SetParams(const std::string& key, const std::string& val)
    {
        _params[key] = val;
    }

    bool HasParams(const std::string& key) const
    {
        return _params.find(key) != _params.end();
    }

    std::string GetParams(const std::string& key) const
    {
        auto it = _params.find(key);
        return it == _params.end() ? "" : it->second;
    }

    void SetPathParam(const std::string& key, const std::string& val)
    {
        _path_params[key] = val;
    }

    bool HasPathParam(const std::string& key) const
    {
        return _path_params.find(key) != _path_params.end();
    }

    std::string GetPathParam(const std::string& key) const
    {
        auto it = _path_params.find(key);
        return it == _path_params.end() ? "" : it->second;
    }

    int ContentLength() const
    {
        if (!HasHeaders("Content-Length"))
            return 0;
        return std::stoi(GetHeaders("Content-Length"));
    }

    bool IsKeepAlive() const
    {
        if (HasHeaders("Connection"))
            return GetHeaders("Connection") == "keep-alive";
        return _http_version == "HTTP/1.1";
    }

    void Reset()
    {
        _method.clear();
        _uri.clear();
        _path.clear();
        _query_string.clear();
        _http_version.clear();
        _body.clear();
        _params.clear();
        _path_params.clear();
        _req_headers.clear();
    }

public:
    std::string _method;
    std::string _uri;
    std::string _path;
    std::string _query_string;
    std::string _http_version;
    std::string _body;
    std::unordered_map<std::string, std::string> _params;
    std::unordered_map<std::string, std::string> _path_params;
    std::unordered_map<std::string, std::string> _req_headers;
};

class HttpResponse
{
public:
    explicit HttpResponse(int code = 200)
        : _code(code)
        , _redirect_flag(false)
    {}

    void SetHeaders(const std::string& key, const std::string& val)
    {
        _headers[key] = val;
    }

    bool HasHeaders(const std::string& key) const
    {
        return _headers.find(key) != _headers.end();
    }

    std::string GetHeaders(const std::string& key) const
    {
        auto it = _headers.find(key);
        return it == _headers.end() ? "" : it->second;
    }

    void SetRedirect(const std::string& redirect_uri, int code = 302)
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

    bool IsKeepAlive() const
    {
        return HasHeaders("Connection") && GetHeaders("Connection") == "keep-alive";
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
    bool RecvReqLine(Buffer* buf)
    {
        if (_status != RECV_REQ_LINE)
            return false;

        if (buf->FindCRLF() == nullptr)
        {
            if (buf->ReadableSize() > MaxLine)
            {
                _resp_code = 414;
                _status = RECV_REQ_ERROR;
                return false;
            }
            return true;
        }

        std::string req_line = buf->GetLineAndPop();
        if (req_line.size() > MaxLine)
        {
            _resp_code = 414;
            _status = RECV_REQ_ERROR;
            return false;
        }

        if (!ParseReqLine(req_line))
        {
            _resp_code = 400;
            _status = RECV_REQ_ERROR;
            return false;
        }

        _status = RECV_REQ_HEAD;
        return true;
    }

    bool ParseReqLine(std::string req_line)
    {
        if (!req_line.empty() && req_line.back() == '\n')
            req_line.pop_back();
        if (!req_line.empty() && req_line.back() == '\r')
            req_line.pop_back();

        size_t pos1 = req_line.find(' ');
        if (pos1 == std::string::npos)
            return false;
        size_t pos2 = req_line.find(' ', pos1 + 1);
        if (pos2 == std::string::npos)
            return false;

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

        _request._path = _request._uri.substr(0, qpos);
        _request._query_string = _request._uri.substr(qpos + 1);
        ParseQueryString();
        return true;
    }

    void ParseQueryString()
    {
        size_t start = 0;
        while (start <= _request._query_string.size())
        {
            size_t end = _request._query_string.find('&', start);
            std::string kv = end == std::string::npos
                ? _request._query_string.substr(start)
                : _request._query_string.substr(start, end - start);

            if (!kv.empty())
            {
                size_t eq_pos = kv.find('=');
                if (eq_pos != std::string::npos)
                {
                    _request.SetParams(Util::UrlDecode(kv.substr(0, eq_pos), true),
                                       Util::UrlDecode(kv.substr(eq_pos + 1), true));
                }
                else
                {
                    _request.SetParams(Util::UrlDecode(kv, true), "");
                }
            }

            if (end == std::string::npos)
                break;
            start = end + 1;
        }
    }

    bool RecvReqHead(Buffer* buffer)
    {
        if (_status != RECV_REQ_HEAD)
            return false;

        while (true)
        {
            if (buffer->FindCRLF() == nullptr)
            {
                if (buffer->ReadableSize() > MaxLine)
                {
                    _status = RECV_REQ_ERROR;
                    _resp_code = 414;
                    return false;
                }
                break;
            }

            std::string line = buffer->GetLineAndPop();
            if (line.size() > MaxLine)
            {
                _status = RECV_REQ_ERROR;
                _resp_code = 414;
                return false;
            }

            if (line == "\r\n" || line == "\n")
            {
                _status = RECV_REQ_BODY;
                return true;
            }

            if (!ParseHttpHead(line))
            {
                _status = RECV_REQ_ERROR;
                _resp_code = 400;
                return false;
            }
        }
        return true;
    }

    bool ParseHttpHead(std::string head)
    {
        if (!head.empty() && head.back() == '\n')
            head.pop_back();
        if (!head.empty() && head.back() == '\r')
            head.pop_back();

        size_t pos = head.find(": ");
        if (pos == std::string::npos)
            return false;

        _request.SetHeaders(head.substr(0, pos), head.substr(pos + 2));
        return true;
    }

    bool RecvHttpBody(Buffer* buf)
    {
        if (_status != RECV_REQ_BODY)
            return false;

        int content_length = _request.ContentLength();
        if (content_length == 0)
        {
            _status = RECV_REQ_OVER;
            return true;
        }

        int cur_length = static_cast<int>(_request._body.size());
        int need_length = content_length - cur_length;
        if (need_length <= 0)
        {
            _status = RECV_REQ_OVER;
            return true;
        }

        size_t readable = buf->ReadableSize();
        if (static_cast<size_t>(need_length) <= readable)
        {
            _request._body += buf->ReadStringAndPop(need_length);
            _status = RECV_REQ_OVER;
        }
        else if (readable > 0)
        {
            _request._body += buf->ReadStringAndPop(readable);
        }
        return true;
    }

public:
    HttpContext()
        : _resp_code(200)
        , _status(RECV_REQ_LINE)
    {}

    int GetRespCode() const
    {
        return _resp_code;
    }

    HttpRecvStatus GetStatus() const
    {
        return _status;
    }

    HttpRequest& GetRequest()
    {
        return _request;
    }

    void RecvHttpRequest(Buffer* buf)
    {
        switch (_status)
        {
        case RECV_REQ_LINE:
            if (!RecvReqLine(buf))
                return;
        case RECV_REQ_HEAD:
            if (!RecvReqHead(buf))
                return;
        case RECV_REQ_BODY:
            RecvHttpBody(buf);
            return;
        default:
            return;
        }
    }

    void Reset()
    {
        _resp_code = 200;
        _status = RECV_REQ_LINE;
        _request.Reset();
    }

private:
    int _resp_code;
    HttpRecvStatus _status;
    HttpRequest _request;
};

using route_func = std::function<void(const HttpRequest&, HttpResponse*)>;

struct RouteItem
{
    std::string _pattern;
    route_func _cb;
};

using RouteTable = std::vector<RouteItem>;

class HttpServer
{
private:
    std::string BuildFilePath(const std::string& path) const
    {
        std::string normalized = path.empty() ? "/" : path;
        if (normalized == "/")
            normalized = "/index.html";
        if (normalized.back() == '/')
            normalized += "index.html";
        return _base_dir + normalized;
    }

    void MakeResponse(PtrConnection conn, const HttpRequest& req, HttpResponse* resp)
    {
        if (req.IsKeepAlive())
            resp->SetHeaders("Connection", "keep-alive");
        else
            resp->SetHeaders("Connection", "close");

        if (!resp->_body.empty() && !resp->HasHeaders("Content-Length"))
            resp->SetHeaders("Content-Length", std::to_string(resp->_body.size()));
        else if (resp->_body.empty() && !resp->HasHeaders("Content-Length"))
            resp->SetHeaders("Content-Length", "0");

        if (!resp->_body.empty() && !resp->HasHeaders("Content-Type"))
            resp->SetHeaders("Content-Type", "application/octet-stream");

        if (resp->_redirect_flag)
            resp->SetHeaders("Location", resp->_redirect_uri);

        std::stringstream resp_str;
        std::string version = req._http_version.empty() ? "HTTP/1.1" : req._http_version;
        resp_str << version << " " << resp->_code << " " << Util::GetStatusDesc(resp->_code) << "\r\n";
        for (const auto& e : resp->_headers)
            resp_str << e.first << ": " << e.second << "\r\n";
        resp_str << "\r\n";

        if (req._method != "HEAD")
            resp_str << resp->_body;

        std::string payload = resp_str.str();
        conn->Send(payload.c_str(), payload.size());
    }

    bool IsFileHandler(const HttpRequest& req)
    {
        if (req._method != "GET" && req._method != "HEAD")
            return false;
        if (req._path.empty() || !Util::IsValidPath(req._path))
            return false;

        std::string real_path = BuildFilePath(req._path);
        if (Util::IsDirectory(real_path))
            real_path += "/index.html";
        return Util::IsRegular(real_path);
    }

    bool MatchRoute(const std::string& pattern, const std::string& path,
                    std::unordered_map<std::string, std::string>* params) const
    {
        std::vector<std::string> pattern_segments;
        std::vector<std::string> path_segments;
        Util::Split(pattern, "/", &pattern_segments);
        Util::Split(path, "/", &path_segments);

        if (pattern_segments.size() != path_segments.size())
            return false;

        std::unordered_map<std::string, std::string> matched;
        for (size_t i = 0; i < pattern_segments.size(); ++i)
        {
            const std::string& pattern_segment = pattern_segments[i];
            const std::string& path_segment = path_segments[i];

            if (pattern_segment.size() > 1 && pattern_segment[0] == ':')
            {
                matched[pattern_segment.substr(1)] = Util::UrlDecode(path_segment, false);
                continue;
            }

            if (pattern_segment != path_segment)
                return false;
        }

        *params = std::move(matched);
        return true;
    }

    void FileHandler(const HttpRequest& req, HttpResponse* resp)
    {
        if (!Util::IsValidPath(req._path))
        {
            resp->_code = 403;
            return;
        }

        std::string real_path = BuildFilePath(req._path);
        if (Util::IsDirectory(real_path))
            real_path += "/index.html";

        if (!Util::IsRegular(real_path))
        {
            resp->_code = 404;
            return;
        }

        std::string body;
        if (!Util::ReadFile(real_path, &body))
        {
            resp->_code = 500;
            return;
        }

        resp->_code = 200;
        resp->SetContent(body, Util::GetMime(real_path));
    }

    void Dispatcher(const HttpRequest& req, HttpResponse* resp, const RouteTable& routes)
    {
        for (const auto& route : routes)
        {
            if (route._pattern == req._path)
            {
                route._cb(req, resp);
                return;
            }
        }

        for (const auto& route : routes)
        {
            std::unordered_map<std::string, std::string> params;
            if (route._pattern.find(':') == std::string::npos)
                continue;
            if (!MatchRoute(route._pattern, req._path, &params))
                continue;

            HttpRequest matched_req = req;
            for (const auto& param : params)
                matched_req.SetPathParam(param.first, param.second);
            route._cb(matched_req, resp);
            return;
        }

        resp->_code = 404;
    }

    void Route(const HttpRequest& req, HttpResponse* resp)
    {
        if (req._path.empty())
        {
            resp->_code = 400;
            return;
        }

        if (IsFileHandler(req))
        {
            FileHandler(req, resp);
            return;
        }

        if (req._method == "GET" || req._method == "HEAD")
            Dispatcher(req, resp, _get_routes);
        else if (req._method == "POST")
            Dispatcher(req, resp, _post_routes);
        else if (req._method == "PUT")
            Dispatcher(req, resp, _put_routes);
        else if (req._method == "DELETE")
            Dispatcher(req, resp, _delete_routes);
        else
            resp->_code = 405;
    }

    void ErrorHandler(const HttpRequest&, HttpResponse* resp)
    {
        std::string body;
        body += "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>";
        body += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
        body += "<title>Error</title></head><body>";
        body += std::to_string(resp->_code);
        body += ": ";
        body += Util::GetStatusDesc(resp->_code);
        body += "</body></html>";
        resp->SetContent(body, "text/html; charset=utf-8");
    }

    void OnMessage(PtrConnection conn, Buffer* buffer)
    {
        while (true)
        {
            HttpContext* context = conn->GetContext()->GetValAddr<HttpContext>();
            if (context == nullptr)
            {
                conn->SetContext(HttpContext());
                context = conn->GetContext()->GetValAddr<HttpContext>();
            }

            context->RecvHttpRequest(buffer);
            HttpRequest req = context->GetRequest();
            HttpResponse resp(context->GetRespCode());

            if (context->GetRespCode() >= 400)
            {
                ErrorHandler(req, &resp);
                MakeResponse(conn, req, &resp);
                buffer->clear();
                conn->ShutDown();
                return;
            }

            if (context->GetStatus() != RECV_REQ_OVER)
                return;

            Route(req, &resp);
            if (resp._code >= 400 && resp._body.empty())
                ErrorHandler(req, &resp);
            MakeResponse(conn, req, &resp);

            bool keep_alive = resp.IsKeepAlive();
            context->Reset();
            if (!keep_alive)
            {
                conn->ShutDown();
                return;
            }

            if (buffer->ReadableSize() == 0)
                return;
        }
    }

    void OnConnect(PtrConnection conn)
    {
        conn->SetContext(HttpContext());
    }

public:
    explicit HttpServer(uint16_t port, const std::string& base_dir = "./www")
        : _server(port)
        , _base_dir(base_dir)
    {
        if (!_base_dir.empty() && _base_dir.back() == '/')
            _base_dir.pop_back();
        if (_base_dir.empty())
            _base_dir = ".";

        _server.SetConnectedCallback(std::bind(&HttpServer::OnConnect, this, std::placeholders::_1));
        _server.SetMessageCallback(std::bind(&HttpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    }

    void Get(const std::string& pattern, const route_func& cb)
    {
        AddRoute(pattern, cb, &_get_routes);
    }

    void Post(const std::string& pattern, const route_func& cb)
    {
        AddRoute(pattern, cb, &_post_routes);
    }

    void Put(const std::string& pattern, const route_func& cb)
    {
        AddRoute(pattern, cb, &_put_routes);
    }

    void Delete(const std::string& pattern, const route_func& cb)
    {
        AddRoute(pattern, cb, &_delete_routes);
    }

    void SetThreadCountAndInit(int nums)
    {
        _server.SetThreadCountAndInit(nums);
    }

    void EnableInactiveRelease(int timeout)
    {
        _server.EnableInactiveRelease(timeout);
    }

    void Start()
    {
        _server.Start();
    }

private:
    void AddRoute(const std::string& pattern, const route_func& cb, RouteTable* routes)
    {
        routes->push_back({pattern, cb});
    }

private:
    TcpServer _server;
    std::string _base_dir;
    RouteTable _get_routes;
    RouteTable _put_routes;
    RouteTable _post_routes;
    RouteTable _delete_routes;
};
