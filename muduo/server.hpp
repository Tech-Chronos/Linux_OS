#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

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
#include <sys/timerfd.h>


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
        return ret;
    }

    std::string ReadStringAndPop(uint64_t size)
    {
        assert(size <= ReadableSize());
        std::string ret;
        ret.resize(size);

        ReadAndPop(ret.data(), size);
        return ret;
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

        return ReadString(ch - ReadIndex() + 1);
    }

    std::string GetLineAndPop()
    {
        std::string ret = GetLine();

        MoveReadOffset(ret.size());
        return ret;
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

    void Remove();

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

// // // // // // // // // // // // TimerTask & TimerWheel // // // // // // // // // // // // // // // 
using TimerFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;
class TimerTask
{
public:
    TimerTask(uint64_t id, uint32_t timeout, const TimerFunc& timer_func)
        : _id(id)
        , _timeout(timeout)
        , _timer_func(timer_func)
        , _cancel(false)
    {}

    uint32_t Timeout()
    {
        return _timeout;
    }

    void Cancel()
    {
        _cancel = true;
    }

    void SetRelease(const ReleaseFunc& release_func)
    {
        _release_func = release_func;
    }

    ~TimerTask()
    {
        if (!_cancel)
            _timer_func();
        if (_release_func)
            _release_func();
    }

private:
    uint64_t _id; // 任务id
    uint32_t _timeout; // 啥时候到时间
    TimerFunc _timer_func; // 到时间了该执行的任务
    ReleaseFunc _release_func; // 到时间自动析构，从TimerWheel的哈希表中删除
    bool _cancel; // 是否取消这个定时任务
};

// 时间轮
using SharedTimerTask = std::shared_ptr<TimerTask>;
using WeakTimerTask = std::weak_ptr<TimerTask>;
class TimerWheel
{
private:

    void RemoveFromTimerWheel(uint64_t id)
    {
        if (IfExists(id))
        {
            _timers_record.erase(id);
        }
    }
public:
    TimerWheel()
        : _capacity(60)
        , _tick(0)
        , _wheels(_capacity)
    {}

    bool IfExists(uint64_t id)
    {
        auto it = _timers_record.find(id);
        return it != _timers_record.end();
    }

    void SetTimerTask(uint64_t id, uint32_t timeout, const TimerFunc& timer_func)
    {
        if (!IfExists(id))
        {
            SharedTimerTask s_task = std::make_shared<TimerTask>(id, timeout, timer_func);
            s_task->SetRelease(std::bind(&TimerWheel::RemoveFromTimerWheel, this, id));
            _wheels[(_tick + timeout) % _capacity].push_front(s_task);
            _timers_record[id] = WeakTimerTask(s_task);
        }
    }

    void RefreshTimerTask(uint64_t id)
    {
        if (IfExists(id))
        {
            SharedTimerTask s_task = _timers_record[id].lock();
            if (s_task)
                _wheels[(_tick + s_task->Timeout()) % _capacity].push_front(s_task);
        }
    }

    void RemoveTimerTask(uint64_t id)
    {
        if (IfExists(id))
        {
            SharedTimerTask s_task = _timers_record[id].lock();
            if (s_task)
                s_task->Cancel();
            _timers_record.erase(id);
        }
    }

    void Tick(int nums)
    {
        while (nums--)
        {
            _tick = (_tick  + 1) % _capacity;
            _wheels[_tick].clear();
        }  
    }
private:
    int _capacity; // 时间轮的容量
    int _tick; // 当前指针走到哪里了
    std::unordered_map<uint64_t, WeakTimerTask> _timers_record;  // 如果要更新，如何找到对应的ptr
    std::vector<std::list<SharedTimerTask>> _wheels; // 用二维数组来表示时间轮
};

// // // // // // // // // // // // EventLoop // // // // // // // // // // // // // // // 
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
            ERR_LOG("Wake Up Epoll To Do Pending Error!");
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

    int CreateTimerFd()
    {
        int ret = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (ret < 0)
        {
            ERR_LOG("Create TimerFd Error!");
            exit(-1);
        }

        return ret;
    }

    void SetTimer()
    {
        struct itimerspec new_value;
        bzero(&new_value, sizeof(new_value));
        new_value.it_value.tv_sec = 1;
        new_value.it_value.tv_nsec = 0;

        new_value.it_interval.tv_sec = 1;
        new_value.it_interval.tv_nsec = 0;
        timerfd_settime(_timer_fd, 0, &new_value, nullptr);
    }

    void TimerReadCb()
    {
        uint64_t num;
        int ret = read(_timer_fd, &num, sizeof(num));
        if (ret < 0)
        {
            if (errno == EWOULDBLOCK || errno == EINTR)
                return;
            ERR_LOG("TimerReadCb Error!");
            exit(-1);
        }
        _timer_wheel->Tick(num);
    }
public:
    EventLoop()
        : _thread_id(std::this_thread::get_id())
        , _event_fd(CreateEventFd())
        , _event_channel(_event_fd, this)
        , _timer_fd(CreateTimerFd())
        , _timer_channel(_timer_fd, this)
        , _timer_wheel(std::make_unique<TimerWheel>())
    {
        // 将event fd注册进epoll
        _event_channel.SetReadCB(std::bind(&EventLoop::EventRead, this));
        _event_channel.EnableRead();
        _poll.UpdateEvent(&_event_channel);

        // 将timer fd注册进epoll
        _timer_channel.SetReadCB(std::bind(&EventLoop::TimerReadCb, this));
        _timer_channel.EnableRead();
        _poll.UpdateEvent(&_timer_channel);
        SetTimer();
    }

    bool IsInLoop()
    {
        return _thread_id == std::this_thread::get_id();
    }

    void AssertInLoop()
    {
        assert(_thread_id == std::this_thread::get_id());
    }

    void RunInLoop(const pending_task& task)
    {
        if (IsInLoop())
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

    // 开始循环
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

    void AddTimerTask(uint64_t id, uint32_t timeout, const TimerFunc& timer_func)
    {
        RunInLoop([this, id, timeout, timer_func = std::move(timer_func)]()->void 
                 {
                    _timer_wheel->SetTimerTask(id, timeout, timer_func);
                 });
    }

    void RefreshTimerTask(uint64_t id)
    {
        RunInLoop([this, id]()->void 
                 {
                    _timer_wheel->RefreshTimerTask(id);
                 });
    }

    void RemoveTimerTask(uint64_t id)
    {
        RunInLoop([this, id]()->void 
                 {
                    _timer_wheel->RemoveTimerTask(id);
                 });
    }

    // 这个函数只能在EventLoop线程内部调用，因为有线程安全问题
    bool IfExistTimerTask(uint64_t id)
    {
        return _timer_wheel->IfExists(id);
    }

private:
    std::thread::id _thread_id; // 当前线程的id

    Poll _poll; // epoll对象
    //std::unique_ptr<Channel> _channels; // 用智能指针管理 channel
    std::vector<Channel*> _active_channel; // 有事件发生的 channel

    std::vector<pending_task> _pending; // 任务队列
    std::mutex _task_mtx; // 任务队列的锁

    int _event_fd; // 用于通知任务队列中有任务到来
    Channel _event_channel;

    int _timer_fd; // 用于定时任务
    Channel _timer_channel;
    std::unique_ptr<TimerWheel> _timer_wheel; // 时间轮
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
    _loop->UpdateEvent(this);
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
    _loop->UpdateEvent(this);
}

void Channel::Remove()
{
    _events = 0;
    _loop->RemoveEvent(this);
}

// // // // // // // // // // // // Connection // // // // // // // // // // // // // // // 
// 作用：一个连接的相关操作：对socket、Channel、Buffer
class Connection;
using PtrConnection = std::shared_ptr<Connection>;
using ConnectedCallback = std::function<void(PtrConnection)>;
using MessageCallback = std::function<void(PtrConnection, Buffer*)>;
using AnyEventCallback = std::function<void(PtrConnection)>;
using ClosedCallback = std::function<void(PtrConnection)>;

class Any
{
private:
    void Swap(Any& other)
    {
        std::swap(_holder, other._holder);
    }
public:
    Any()
        : _holder(nullptr)
    {}

    template<class T>
    Any(const T& val)
        : _holder(new PlaceHolder<T>(val))
    {}

    Any(const Any& other)
    {
        _holder = other._holder == nullptr ? nullptr : other._holder->Clone();
    }


    template <class T>
    Any& operator=(const T& val)
    {
        Any(val).Swap(*this);
        return *this;
    }

    Any& operator=(Any other)
    {
        Swap(other);
        return *this;
    }

    template <class T>
    T* GetValAddr()
    {
        if (_holder && typeid(T) == _holder->GetType())
        {
            return &((PlaceHolder<T>*)_holder)->_val;
        }
        return nullptr;
    }

    ~Any()
    {
        delete _holder;
    }

private:
    class Holder
    {
    public:
        virtual ~Holder() { }

        virtual const std::type_info& GetType() = 0;

        virtual Holder* Clone() = 0;
    };

    template<class T>
    class PlaceHolder : public Holder
    {
    public:
        PlaceHolder(const T& val)
            :_val(val)
        {}

        const std::type_info& GetType() override
        {
            return typeid(T);
        }

        Holder* Clone() override
        {
            return new PlaceHolder<T>(_val);
        }
    public:
        T _val;
    };

    // 基类指针指向派生类
    Holder* _holder;
};

typedef enum 
{ 
    DISCONNECTED, 
    CONNECTING, 
    CONNECTED, 
    DISCONNECTING 
} ConnStatus;

class Connection : public std::enable_shared_from_this<Connection>
{
private:
    // // // // // // // // // // // // 五个事件处理回调函数 // // // // // // // // // // // // // // // 
    // 当事件来的时候，绑定对应的函数
    void HandleRead()
    {
        if (_status == DISCONNECTED) return;
        auto self = shared_from_this();
        // 先从socket中读数据，放到inbuffer
        char buffer[65536];
        int ret = _socket.RecvNonBlock(buffer, sizeof(buffer));
        if (ret == -2) // 读到EAGAIN
            return; 
        else if (ret == -1) // 读出错
        {
            return ShutDownInLoop(); 
        }
        else if (ret == 0)
        {
            return ShutDownInLoop();
        }
        else if (ret > 0)
        {
            _in_buffer.WriteAndPush(buffer, ret);
        }

        // 然后调用服务器传过来的业务处理函数
        if (_in_buffer.ReadableSize() > 0)
            if (_mess_cb)
                _mess_cb(shared_from_this(), &_in_buffer);
    }

    void HandleWrite()
    {
        auto self = shared_from_this();
        std::string str = _out_buffer.ReadStringAndPop(_out_buffer.ReadableSize());
        
        int ret = _socket.SendNonBlock(str.c_str(), str.size());
        // 写错误，先处理接收缓冲区中的数据，然后关闭连接
        if (ret == -1)
        {
            if (_in_buffer.ReadableSize() > 0)
                if (_mess_cb)
                    _mess_cb(self, &_in_buffer);
            ReleaseInLoop();
        }

        if (_out_buffer.ReadableSize() == 0)
        {
            _channel.DisableWrite(); 
            if (_status == DISCONNECTING)
                ReleaseInLoop();
        }
    }
    
    void HandleClose()
    {
        auto self = shared_from_this();
        if (_in_buffer.ReadableSize() > 0)
            if (_mess_cb)
                _mess_cb(shared_from_this(), &_in_buffer);
        ReleaseInLoop();
    }

    void HandleError()
    {
        HandleClose();
    }

    void HandleAnyEvent()
    {
        auto self = shared_from_this();
        if (_any_event_cb)
            _any_event_cb(self);
        
        if (_enable_inactive_release)
            _loop->RefreshTimerTask(_timer_id);
    }

    // // // // // // // // // // // // 放到任务队列中的任务 // // // // // // // // // // // // // // // 
    // 连接到来的时候
    void EstablishedInLoop()
    {
        assert(_status == CONNECTING);
        _status = CONNECTED;

        // 启动读事件
        _channel.EnableRead();

        // 调用服务器传过来的函数
        if (_conn_cb)
            _conn_cb(shared_from_this());
    }

    // 放到缓冲区，不是真正的发送
    void SendInLoop(const char* message, size_t len)
    {
        if (_status == DISCONNECTED)
            return;

        _out_buffer.WriteAndPush(message, len);
        if (!_channel.Writeable())
            _channel.EnableWrite();
    }

    void ShutDownInLoop()
    {
        _status = DISCONNECTING;

        if (_in_buffer.ReadableSize() > 0)
        {
            if (_mess_cb)
                _mess_cb(shared_from_this(), &_in_buffer);
        }
            
        if (_out_buffer.ReadableSize() > 0)
        {
            if (!_channel.Writeable())
                _channel.EnableWrite();
        }

        if (_out_buffer.ReadableSize() == 0)
        {
            ReleaseInLoop();
        }
    }

    void EnableInactiveReleaseInLoop(int sec)
    {
        _enable_inactive_release = true;

        if (_loop->IfExistTimerTask(_timer_id))
        {
            _loop->RefreshTimerTask(_timer_id);
        }
        else
        {
            std::weak_ptr<Connection> weak_con = shared_from_this();
            _loop->AddTimerTask(_timer_id, sec, [weak_con]() -> 
            void 
            { 
                auto shared_con = weak_con.lock();
                if (shared_con)
                    shared_con->ReleaseInLoop();
            });
        }
    }

    void CancelInactiveReleaseInLoop()
    {
        _enable_inactive_release = false;

        if (_loop->IfExistTimerTask(_timer_id))
        {
            _loop->RemoveTimerTask(_timer_id);
        }
    }

    void ReleaseInLoop()
    {
        _status = DISCONNECTED;

        _channel.Remove();

        _socket.Close();

        if (_loop->IfExistTimerTask(_timer_id))
        {
            CancelInactiveReleaseInLoop();
        }

        if (_closed_cb)
            _closed_cb(shared_from_this());
        if (_server_close_cb)
            _server_close_cb(shared_from_this());
    }

    void UpgradeInLoop(const Any& context, const ConnectedCallback& conn_cb,
                const MessageCallback& mess_cb, const AnyEventCallback& any_event_cb,
                const ClosedCallback& closed_cb)
    {
        _context = context;
        _conn_cb = conn_cb;
        _mess_cb = mess_cb;
        _any_event_cb = any_event_cb;
        _closed_cb = closed_cb;
    }
public:
    Connection(uint64_t conn_id, uint64_t timer_id, int sockfd, EventLoop* loop)
        : _conn_id(conn_id)
        , _timer_id(timer_id)
        , _sockfd(sockfd)
        , _enable_inactive_release(false)
        , _loop(loop)
        , _socket(_sockfd)
        , _channel(_sockfd, _loop)
        , _status(CONNECTING)
    {
        _channel.SetReadCB([this] { HandleRead(); });
        _channel.SetWriteCB([this] { HandleWrite(); });
        _channel.SetAnyEventCB([this] { HandleAnyEvent(); });
        _channel.SetCloseCB([this] { HandleClose(); });
        _channel.SetErrorCB([this] { HandleError(); });
    }

    ~Connection()
    {
        DBG_LOG("Release Connection : %p", this);
    }

    // // // // // // // // // // // // 向外部提供的接口 // // // // // // // // // // // // // // // 
    void Established()
    {
        auto self = shared_from_this();
        _loop->RunInLoop([self] { self->EstablishedInLoop(); });
    }

    // 发送数据，将这个数据放到发送缓冲区
    void Send(const char* message, size_t len)
    {
        auto self = shared_from_this();
        _loop->RunInLoop([self, message, len]{ self->SendInLoop(message, len);});
    }

    // 先检查接收和发送缓冲区是否有数据，有线进行处理，处理完，在关闭
    void ShutDown()
    {
        auto self = shared_from_this();
        _loop->RunInLoop([self]{  self->ShutDownInLoop();});
    }

    // 开启非活跃连接的释放功能
    void EnableInactiveRelease(int sec)
    {
        auto self = shared_from_this();
        _loop->RunInLoop([self, sec]{  self->EnableInactiveReleaseInLoop(sec); });
    }

    // 关闭非活跃事件的释放功能
    void CancelInactiveRelease()
    {
        auto self = shared_from_this();
        _loop->RunInLoop([self]{  self->CancelInactiveReleaseInLoop(); });
    }

    void Upgrade(const Any& context, const ConnectedCallback& conn_cb,
                const MessageCallback& mess_cb, const AnyEventCallback& any_event_cb,
                const ClosedCallback& closed_cb)
    {
        _loop->AssertInLoop();

        _loop->RunInLoop( [=] { UpgradeInLoop(context, conn_cb, mess_cb, any_event_cb, closed_cb); } );
    }

    uint64_t TimerId()
    {
        return _timer_id;
    }

    uint64_t ConnId()
    {
        return _conn_id;
    }

    // 设置回调函数
    void SetConnectedCallback(const ConnectedCallback& cb)
    {
        _conn_cb = cb;
    }

    void SetMessageCallback(const MessageCallback& cb)
    {
        _mess_cb = cb;
    }

    void SetAnyEventCallback(const AnyEventCallback& cb)
    {
        _any_event_cb = cb;
    }

    void SetClosedCallback(const ClosedCallback& cb)
    {
        _closed_cb = cb;
    }

    void SetServerClosedCallback(const ClosedCallback& cb)
    {
        _server_close_cb = cb;
    }

    Any* GetContext()
    {
        return &_context;
    }

    void SetContext(Any context)
    {
        _context = context;
    }
private:
    uint64_t _conn_id; // 连接id
    uint64_t _timer_id; // 定时器id
    int _sockfd; // 这个连接的文件描述符
    bool _enable_inactive_release; // 非活跃事件的开启 or 关闭 
    Any _context; // 当前连接的协议

    EventLoop* _loop; // 这个连接属于哪一个loop
    Socket _socket; // 这个连接的文件描述符的相关操作
    Channel _channel; // 这个连接事件触发的时候如何执行
    Buffer _in_buffer; // 接收缓冲区
    Buffer _out_buffer; // 发送缓冲区
    ConnStatus _status; // 这个连接的状态  
    
    // 阶段处理函数
    ConnectedCallback _conn_cb; // 连接到来处理函数
    MessageCallback _mess_cb; // 事件处理回调函数
    AnyEventCallback _any_event_cb; // 任意事件处理回调函数
    ClosedCallback _closed_cb; // 连接关闭回调函数
    ClosedCallback _server_close_cb; 
};

// // // // // // // // // // // // Acceptor模块 // // // // // // // // // // // // // // // 
using AcceptorCallback = std::function<void(int)>; // 新的IO文件描述符到来时需要执行的回调函数
class Acceptor
{
private:
    int CreateListenSocket(uint16_t port)
    {
        _listen_sock.CreateTcpSocket(port);
        return _listen_sock.Fd();
    }

    void HandleListenRead()
    {
        int io_fd = _listen_sock.Accept();
        if (io_fd == -2)
        {
            return;
        }
        if (io_fd < 0)
        {
            ERR_LOG("Recv New Socket Error!");
            exit(-1);
        }

        if (_accept_cb)
            _accept_cb(io_fd);
    }
public:
    Acceptor(EventLoop* loop, uint16_t port)
        : _main_loop(loop)
        , _listen_fd(CreateListenSocket(port))
        , _listen_channel(_listen_fd, _main_loop)
    {
       _listen_channel.SetReadCB([this]{ HandleListenRead(); });
    }

    void Listen()
    {
        _listen_channel.EnableRead();
    }

    void SetAcceptCallback(const AcceptorCallback& cb)
    {
        _accept_cb = cb;
    }

private:
    EventLoop* _main_loop;
    Socket _listen_sock;
    int _listen_fd;
    Channel _listen_channel;
    AcceptorCallback _accept_cb;
};

// // // // // // // // // // // // LoopThread // // // // // // // // // // // // // // // 
class LoopThread
{
private:
    // 创建EventLoop对象
    void ThreadEntry()
    {
        EventLoop loop;
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _loop = &loop;
            _cond.notify_all();
        }
        _loop->Loop();
    }
public:
    LoopThread() 
        : _loop(nullptr)
        , _thread(&LoopThread::ThreadEntry, this)
    {}

    EventLoop* GetLoopPtr()
    {
        // 防止如果eventloop还没有构造，外部就要获取
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _cond.wait(lock, [this] { return _loop != nullptr; });
        }
        return _loop;
    }

private:
    EventLoop* _loop; // 这个线程对应的 EventLoop
    std::thread _thread;

    std::mutex _mtx; // 防止如果eventloop还没有构造，外部就要获取
    std::condition_variable _cond; // 所以用条件变量等待
};


// // // // // // // // // // // // LoopThreadPool // // // // // // // // // // // // // // // 
class LoopThreadPool
{
public:
    LoopThreadPool(EventLoop* base_loop)
        : _base_loop(base_loop)
    {}

    void SetThreadSize(int threads_size)
    {
        _threads_size = threads_size;
    }

    void InitPool()
    {
        if (_threads_size > 0)
        {
            _threads.reserve(_threads_size);
            _loops.reserve(_threads_size);

            for (int i = 0; i < _threads_size; ++i)
            {
                auto t = std::make_unique<LoopThread>();
                _loops.push_back(t->GetLoopPtr());
                _threads.push_back(std::move(t));
            }
        }   
    }

    EventLoop* GetNextLoopThread()
    {
        if (_threads_size > 0)
        {
            _thread_index = (_thread_index + 1) % _threads_size;

            return _loops[_thread_index];
        }
        return _base_loop;
    }

private:
    int _threads_size = 0;
    int _thread_index = 0;
    EventLoop* _base_loop;
    std::vector<std::unique_ptr<LoopThread>> _threads;
    std::vector<EventLoop*> _loops;
};

// // // // // // // // // // // // TcpServer模块 // // // // // // // // // // // // // // // 
class TcpServer
{
private:
    void AcceptCallback(int io_fd)
    {
        ++_conn_id;
        ++_timer_id;
        auto conn = std::make_shared<Connection>(_conn_id, _timer_id, io_fd, _pool.GetNextLoopThread()); 
        conn->SetConnectedCallback(_conn_cb);
        conn->SetAnyEventCallback(_any_event_cb);
        conn->SetMessageCallback(_message_cb);
        conn->SetClosedCallback(_close_cb);
        conn->SetServerClosedCallback(std::bind(&TcpServer::RemoveFromServer, this, std::placeholders::_1));

        conn->Established();

        if (_enable_inactive_event_release)
            conn->EnableInactiveRelease(_timeout);

        _conns[_conn_id] = conn;
    }

    void RemoveFromServer(std::shared_ptr<Connection> conn)
    {
        _main_loop.RunInLoop([this, conn]
        {
            auto it = _conns.find(conn->ConnId());
            if (it != _conns.end())  _conns.erase(it); 
        });
    }
public:
    TcpServer(int port)
        : _port(port)
        , _accept(&_main_loop, _port)
        , _pool(&_main_loop)
    {
        _accept.SetAcceptCallback(std::bind(&TcpServer::AcceptCallback, this, std::placeholders::_1));
        _accept.Listen();
    }

    void SetThreadCountAndInit(int nums)
    {
        _threads_size = nums;
        _pool.SetThreadSize(_threads_size);
        _pool.InitPool();
    }

    void Start()
    {
        _main_loop.Loop();
    }

    void EnableInactiveRelease(int timeout)
    {
        _enable_inactive_event_release = true;
        _timeout = timeout;
    }

    // 设置回调函数
    void SetConnectedCallback(const ConnectedCallback& cb)
    {
        _conn_cb = cb;
    }

    void SetMessageCallback(const MessageCallback& cb)
    {
        _message_cb = cb;
    }

    void SetAnyEventCallback(const AnyEventCallback& cb)
    {
        _any_event_cb = cb;
    }

    void SetClosedCallback(const ClosedCallback& cb)
    {
        _close_cb = cb;
    }


private:
    EventLoop _main_loop;
    int _port;
    Acceptor _accept;

    std::unordered_map<uint64_t, PtrConnection> _conns;
    LoopThreadPool _pool;

    int _conn_id = 0;
    int _timer_id = 0;
    int _threads_size = 0;
    bool _enable_inactive_event_release = false; // 是否开启非活跃事件的释放
    int _timeout = 0;

    ConnectedCallback _conn_cb;
    AnyEventCallback _any_event_cb;
    ClosedCallback _close_cb;
    ClosedCallback _server_close_cb;
    MessageCallback _message_cb;
};



