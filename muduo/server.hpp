#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>

#include <ctime>
#include <cstdint>
#include <cassert>
#include <cstring>

#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>


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
const static int events_size = 1024;
const static int pending_task_size = 32;

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
        INF_LOG("Create Socket Success, fd = %d", _sockfd);
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
            else // 非阻塞
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

    int Send(const char* message, int size, int flag)
    {
        int ret = send(_sockfd, message, size, flag);
        if (ret < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
            {
                INF_LOG("No Send Event!");
                return -2;
            }
            else
            {
                ERR_LOG("Send Error!");
                return -1;
            }
        }
        //INF_LOG("Send Success!");
        return ret;
    }

    int SendNonBlock(const char* message, int size)
    {
        int ret = Send(message, size, MSG_DONTWAIT);
        return ret;
    }

    void Close()
    {
        if (_sockfd > 0)
        {
            ::close(_sockfd);
            _sockfd = -1;
        }
    }

    int Fd()
    {
        return _sockfd;
    }
private:
    int _sockfd;
};

// // // // // // // // // // // // Channel // // // // // // // // // // // // // // // 
class EventLoop;
using EventCallbck = std::function<void()>;
// 每个文件描述符关心什么事件，事件来的时候怎么处理
class Channel
{
public:
    Channel(int fd, EventLoop* loop)
        : _fd(fd)
        , _events(0)
        , _revents(0)
        , _loop(loop)
    {}

    int Fd()
    {
        return _fd;
    }

    uint32_t Events()
    {
        return _events;
    }

    void SetRevents(uint32_t revents)
    {
        _revents = revents;
    }

    // // // // // // // // // // // // 读事件 // // // // // // // // // // // // // // // 
    // 判断fd是否可读
    bool Readable()
    {
        return _events & EPOLLIN;
    }

    // 启动可读事件，在epoll中
    void EnableRead();

    // 删除读事件
    void DisableRead();

    // // // // // // // // // // // // 写事件 // // // // // // // // // // // // // // // 
    // 判断fd是否可写
    bool Writeable()
    {
        return _events & EPOLLOUT;
    }
   
    // 启动可写事件，在epoll中
    void EnableWrite();

    void DisableWrite();

    // // // // // // // // // // // // 设置回调 // // // // // // // // // // // // // // // 
    void SetReadCB(const EventCallbck& cb)
    {
        _read_cb = cb;
    }

    void SetWriteCB(const EventCallbck& cb)
    {
        _write_cb = cb;
    }

    void SetErrorCB(const EventCallbck& cb)
    {
        _error_cb = cb;
    }

    void SetAnyEventCB(const EventCallbck& cb)
    {
        _any_event_cb = cb;
    }

    void SetCloseCB(const EventCallbck& cb)
    {
        _close_cb = cb;
    }
    // // // // // // // // // // // // 事件处理 // // // // // // // // // // // // // // // 
    void HandleEvents()
    {
        if (_any_event_cb)
            _any_event_cb();
        
        if (_revents & EPOLLHUP)
        {
            if (_close_cb)
                _close_cb();
            return;
        }
        if (_revents & EPOLLERR)
        {
            if (_error_cb)
                _error_cb();
            return;
        }
            
        if (_revents & (EPOLLIN | EPOLLPRI))
        {
            if (_read_cb)
                _read_cb();
            return;
        }
            
        if (_revents & EPOLLOUT)
        {
            if (_write_cb)
                _write_cb();
            return;
        }
    }

private:
    int _fd;
    uint32_t _events;
    uint32_t _revents;

    EventCallbck _read_cb, _write_cb, _error_cb, _any_event_cb, _close_cb;

    EventLoop* _loop; // 属于那个 EventLoop
};

// // // // // // // // // // // // Poller // // // // // // // // // // // // // // // 
class Poll
{
private:
    bool IfExist(int fd)
    {
        return _care_fd.find(fd) != _care_fd.end();
    }

    void UpdateMonitor(Channel* ch, int flag)
    {
        int fd = ch->Fd();

        epoll_event ev;
        ev.events = ch->Events();
        ev.data.fd = fd;

        if (epoll_ctl(_epfd, flag, fd, &ev) < 0)
        {
            ERR_LOG("Epoll Ctl Error! flag = %d", flag);
            exit(-1);
        }
    }
public:
    Poll()
    {
        _epfd = epoll_create(events_size);
        if (_epfd < 0)
        {
            ERR_LOG("Epoll Create Error!");
            std::exit(-1);
        }
    }

    void UpdateEvent(Channel* ch)
    {
        int fd = ch->Fd();

        if (!IfExist(fd))
        {
            UpdateMonitor(ch, EPOLL_CTL_ADD);
            _care_fd[fd] = ch;
        }
        else
        {
            UpdateMonitor(ch, EPOLL_CTL_MOD);
        }
    }

    void RemoveEvent(Channel* ch)
    {
        if (IfExist(ch->Fd()))
        {
            epoll_ctl(_epfd, EPOLL_CTL_DEL, ch->Fd(), nullptr);
            _care_fd.erase(ch->Fd());
        }
    }

    // 进行监控
    void Polling(std::vector<Channel*>& _active_channel)
    {
        int ret = epoll_wait(_epfd, _evs, events_size, -1);
        if (ret < 0)
        {
            if (errno == EINTR)
                return;
            ERR_LOG("Epoll Wait Error!");
            exit(-1);
        }

        for (int i = 0; i < ret; ++i)
        {
            struct epoll_event ev = _evs[i];
            if (IfExist(ev.data.fd))
            {
                Channel* ch = _care_fd[ev.data.fd];
                ch->SetRevents(ev.events);
                _active_channel.push_back(ch);
            }
        }
    }
private:
    int _epfd;
    struct epoll_event _evs[events_size];

    std::unordered_map<int, Channel*> _care_fd; // 哈希表管理是关心的那些文件描述符
};

// // // // // // // // // // // // EventLop // // // // // // // // // // // // // // // 
using pending_task = std::function<void()>;
class EventLoop
{
private:
    void DoPendingTask()
    {
        std::vector<pending_task> pending;
        {
            std::unique_lock<std::mutex> lock(_task_mtx);
            _pending.swap(pending);
        }

        for (auto& pend : pending)
        {
            pend();
        }
    }

    int CreateEventFd()
    {
        int ret = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (ret < 0)
        {
            ERR_LOG("Create Event Fd Error!");
            exit(-1);
        }
        return ret;
    }

    void WakeUpEpollToDoPending()
    {
        uint64_t num = 1;
        int ret = write(_event_fd, &num, sizeof(num));
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            ERR_LOG("WakeUpEpollToDoPending Error!");
            exit(-1);
        }
    }

    void EventRead()
    {
        uint64_t num;
        int ret = read(_event_fd, &num, sizeof(num));
        if (ret < 0)
        {
            if (errno == EWOULDBLOCK || errno == EINTR)
                return;
            ERR_LOG("EventFdCb Error!");
            exit(-1);
        }
    }
public:
    EventLoop()
        : _thread_id(std::this_thread::get_id())
        , _event_fd(CreateEventFd())
        , _event_channel(_event_fd, this)
    {
        _event_channel.SetReadCB(std::bind(&EventLoop::EventRead, this));

        _event_channel.EnableRead();
        _poll.UpdateEvent(&_event_channel);
    }

    bool IsInLoop()
    {
        return _thread_id == std::this_thread::get_id();
    }

    void RunInLoop(const pending_task& task)
    {
        if (!IsInLoop())
        {
            task();
            return;
        }
        QueueInLoop(task);
        WakeUpEpollToDoPending();
    }

    void QueueInLoop(const pending_task& task)
    {
        std::unique_lock<std::mutex> lock(_task_mtx);
        _pending.push_back(task);
    }
    
    // 插入/更新事件
    void UpdateEvent(Channel* channel) 
    {
        _poll.UpdateEvent(channel);
    }

    // 删除事件
    void RemoveEvent(Channel* channel)
    {
        _poll.RemoveEvent(channel);
    }

    void Loop()
    {
        while (true)
        {
            _active_channel.clear();
            _poll.Polling(_active_channel);
            for (auto& channel : _active_channel)
            {
                channel->HandleEvents();
            }
            DoPendingTask();
        }
    }

private:
    std::thread::id _thread_id; // 当前线程的id

    Poll _poll; // epoll对象
    std::unique_ptr<Channel> _channels; // 用智能指针管理 channel
    std::vector<Channel*> _active_channel; // 有事件发生的 channel

    std::vector<pending_task> _pending; // 任务队列
    std::mutex _task_mtx; // 任务队列的锁

    int _event_fd; // 用于通知任务队列中有任务到来
    Channel _event_channel;
};

// 启动读事件
void Channel::EnableRead()
{
    _events |= EPOLLIN;
    _loop->UpdateEvent(this);
}

// 删除读事件
void Channel::DisableRead()
{
    _events &= ~EPOLLIN;
    _loop->RemoveEvent(this);
}

// 启动可写事件，在epoll中
void Channel::EnableWrite()
{
    _events |= EPOLLOUT;
    _loop->UpdateEvent(this);
}

void Channel::DisableWrite()
{
    _events &= ~EPOLLOUT;
    _loop->RemoveEvent(this);
}


