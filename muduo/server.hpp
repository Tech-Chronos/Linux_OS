#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include <ctime>
#include <cstdint>
#include <cassert>
#include <cstring>

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>


#define LOG_INF 0
#define LOG_DBG 1
#define LOG_ERR 2

#define LOG(level, format, ...)                                                            \
    do                                                                                     \
    {                                                                                      \
        time_t curtime = time(nullptr);                                                    \
        struct tm *local = localtime(&curtime);                                            \
        char tmp[32] = {0};                                                                \
        strftime(tmp, 31, "%H:%M:%S", local);                                              \
        fprintf(stdout, "[%p %s][%s:%d]" format "\n", (void*)pthread_self(), tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define INF_LOG(format, ...) LOG(LOG_INF, format, ##__VA_ARGS__)
#define DBG_LOG(format, ...) LOG(LOG_DBG, format, ##__VA_ARGS__)
#define ERR_LOG(format, ...) LOG(LOG_ERR, format, ##__VA_ARGS__)

const static uint64_t default_size = 1024;
const static int default_back_log = 4;

class Buffer
{
public:
    Buffer()
        : _buffer(default_size)
        , _write_idx(0)
        , _read_idx(0)
    {}

    // 写相关
    // 获取写偏移
    char* WriteIndex() 
    {
        return _buffer.data() + _write_idx;
    }

    // 获取后沿空间大小
    uint64_t GetAfterSpace() 
    {
        return _buffer.size() - _write_idx;
    }

    // 获取前沿空间大小
    uint64_t GetFrontSpace() 
    {
        return _read_idx;
    }

    // 保证有充足的写空间
    void EnsureAmpleWriteableSpace(uint64_t size)
    {
        uint64_t after = GetAfterSpace();
        uint64_t front = GetFrontSpace();

        if (after >= size) return;
        else if (front + after >= size)
        {
            // 先记录一下可读空间大小
            uint64_t origin_read_size = ReadableSize();

            std::copy(_buffer.data() + _read_idx, _buffer.data() + _write_idx, _buffer.data());
            _read_idx = 0;
            _write_idx = origin_read_size;
        }
        else
        {
            _buffer.resize(_buffer.size() * 2 + size);
        }
    }

    // 移动写偏移
    void MoveWriteOffset(uint64_t size)
    {
        assert(_write_idx + size <= _buffer.size());
        _write_idx += size;
    }

    // // // // // // // // // // // // // 下面是读相关 // // // // // // // // // // // // // // // 

    // 获取读偏移
    char* ReadIndex()
    {
        return _buffer.data() +_read_idx;
    }

    // 获取可读数据大小
    uint64_t ReadableSize()
    {
        return _write_idx - _read_idx;
    }

    // 移动读偏移
    void MoveReadOffset(uint64_t size)
    {
        assert(_read_idx + size <= _write_idx && size > 0);
        _read_idx += size;
    }

    // // // // // // // // // // // // // 真正的写入 // // // // // // // // // // // // // // //
    // 写入const char* 
    void Write(const char* data, uint64_t len)
    {
        EnsureAmpleWriteableSpace(len);
        std::copy(data, data + len, WriteIndex());
    }

    void WriteAndPush(const char* data, uint64_t len)
    {
        Write(data, len);
        MoveWriteOffset(len);
    }

    // 写入string
    void WriteString(const std::string& str)
    {
        Write(str.c_str(), str.size());
    }

    void WriteStringAndPush(const std::string& str)
    {
        WriteAndPush(str.c_str(), str.size());
    }

    // 写入 Buffer
    Buffer& Swap(Buffer& buf)
    {
        std::swap(buf._buffer, _buffer);
        std::swap(buf._read_idx, _read_idx);
        std::swap(buf._write_idx, _write_idx);

        return *this;
    }

    void WriteBuffer(Buffer& other)
    {
        Write(other.ReadIndex(), other.ReadableSize());
    }

    void WriteBufferAndPush(Buffer& other)
    {
        WriteAndPush(other.ReadIndex(), other.ReadableSize());
    }

    // // // // // // // // // // // // // 真正的读取 // // // // // // // // // // // // // // //
    void Read(void* buf, uint64_t size)
    {
        assert(size <= ReadableSize());

        std::copy(ReadIndex(), ReadIndex() + size, (char*)buf);
    }

    void ReadAndPop(void* buf, uint64_t size)
    {
        Read(buf, size);
        MoveReadOffset(size);
    }

    std::string ReadString(uint64_t size)
    {
        assert(size <= ReadableSize());
        std::string ret;
        ret.resize(size);

        Read(ret.data(), size);
        return std::move(ret);
    }

    std::string ReadStringAndPop(uint64_t size)
    {
        assert(size <= ReadableSize());
        std::string ret;
        ret.resize(size);

        ReadAndPop(ret.data(), size);
        return std::move(ret);
    }

    // // // // // // // // // // // // // 获取一整行数据 // // // // // // // // // // // // // // //
    const char* FindCRLF()
    {
        return (const char*)memchr(ReadIndex(), '\n', ReadableSize());
    }

    std::string GetLine()
    {
        const char* ch = FindCRLF();
        if (!ch) return "";

        return std::move(ReadString(ch - ReadIndex() + 1));
    }

    std::string GetLineAndPop()
    {
        std::string ret = GetLine();

        MoveReadOffset(ret.size());
        return std::move(ret);
    }

    void clear()
    {
        _write_idx = _read_idx = 0;
    }
private:
    std::vector<char> _buffer;
    uint64_t _write_idx;
    uint64_t _read_idx;
};


class Socket
{
private:
    void SetNonBlock(int fd)
    {
        int flag = fcntl(fd, F_GETFL);
        if (flag < 0)
        {
            ERR_LOG("Fcntl Error!");
            return;
        }
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    }

    void ReuseAddrAndPort()
    {
        int opt = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // timewait 能绑定这个IP + port
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)); // 这个端口能被多个进程/线程绑定
    }
public:
    Socket()
        : _sockfd(-1)
    {}

    Socket(int sockfd)
        : _sockfd(sockfd)
    {}

    bool CreateTcpSocket(uint16_t port, int back_log = default_back_log)
    {
        if (CreateSocket() == false) return false;
        SetNonBlock(_sockfd);
        ReuseAddrAndPort();
        if (Bind(port) == false) return false;
        if (Listen(back_log) == false) return false; 

        return true;
    }

    bool CreateClient(const std::string& ip, uint16_t port)
    {
        if (CreateSocket() == false) return false;
        if (Connect(ip, port) == false) return false;
        return true;
    }

    bool CreateSocket()
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sockfd < 0) 
        {
            ERR_LOG("Create Socket Error!");
            return false;
        }
        DBG_LOG("Create Socket Success, fd = %d", _sockfd);
        return true;
    }

    // 绑定本地端口
    bool Bind(uint16_t port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            ERR_LOG("Bind Error!");
            return false;
        }
        INF_LOG("Server Bind Success!");
        return true;
    }

    // 监听
    bool Listen(int back_log = default_back_log)
    {
        if (listen(_sockfd, back_log) < 0)
        {
            ERR_LOG("Listen Error!");
            return false;
        }
        INF_LOG("Server Is Listening!");
        return true;
    }

    // 接收
    int Accept()
    {
        // 如果设置了非阻塞，就要判断是否读到的 EAGAIN
        int io_fd = accept(_sockfd, nullptr, nullptr);
        if (io_fd < 0)
        {
            // 单纯 的错了
            if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
            {
                ERR_LOG("Accept Error");
                return -1;
            }
            else
            {
                INF_LOG("No Client Connect!");
                return -2;
            }
        }
        INF_LOG("Client Connect Success!");
        SetNonBlock(io_fd);
        return io_fd;
    }

    // 客户端连接
    bool Connect(const std::string& server_ip, uint16_t port)
    {
        struct sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr);
        addr.sin_port = htons(port);

        if (connect(_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            ERR_LOG("Connect Error!");
            return false;
        }

        INF_LOG("Connect Success!");
        return true;
    }

    int Recv(char* buffer, size_t size, int flag)
    {
        int ret = recv(_sockfd, buffer, size, flag);
        if (ret < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
            {
                INF_LOG("No Read Event!");
                return -2;
            }
            else
            {
                ERR_LOG("Recv Error!");
                return -1;
            }
        }
        else if (ret == 0)
        {
            INF_LOG("Client Quit!");
            return 0;
        }
        buffer[ret] = 0;
        return ret;
    }

    int RecvNonBlock(char* buffer, size_t size)
    {
        int ret = Recv(buffer, size, MSG_DONTWAIT);
        return ret;
    }

private:
    int _sockfd;
};


