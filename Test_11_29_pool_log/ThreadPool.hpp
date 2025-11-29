#include "Thread.hpp"

template <class F>
class ThreadPool
{
private:
    bool IsEmpty()
    {
        return _tasks_queue.empty();
    }
    void Lock()
    {
        pthread_mutex_lock(&_mutex);
    }

    void Unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }

    bool Wait()
    {
        return pthread_cond_wait(&_cond, &_mutex);
    }

    void WakeUp()
    {
        pthread_cond_signal(&_cond);
    }

    void WakeUpAll()
    {
        pthread_cond_broadcast(&_cond);
    }

    void HandlerTask(std::string &name)
    {
        while (true)
        {
            Lock();
            // 但凡队伍里面有任务，就不能退出，必须执行完所有的任务才能退出
            // while 防止假唤醒
            while (IsEmpty() && _is_running)
            {
                ++_sleep_threads_num;
                Wait();
                --_sleep_threads_num;
            }

            // 退出条件
            if (IsEmpty() && !_is_running)
            {
                Unlock();
                break;
            }

            // 有任务
            F task = _tasks_queue.front();
            _tasks_queue.pop();
            Unlock();
            task(name);
        }
    }

public:
    ThreadPool(int thread_num)
        : _thread_num(thread_num), _sleep_threads_num(0), _is_running(false)
    {
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_cond,nullptr);
    }

    void Init()
    {
        _is_running = true;

        for (int i = 0; i < _thread_num; ++i)
        {
            std::string name = "thread [" + std::to_string(i + 1) + "]";
            task_t task = std::bind(&ThreadPool::HandlerTask, this, std::placeholders::_1);
            _threads.emplace_back(name, task);
        }
    }

    void Start()
    {
        _is_running = true;
        for (auto &thread : _threads)
        {
            thread.Start();
        }
    }

    void Enqueue(const F func)
    {
        Lock();

        // 防止线程退出之后继续在任务队列中插入任务
        if (_is_running)
        {
            _tasks_queue.push(func);
            // 这边插入任务之后要通知线程有任务了
            if (_sleep_threads_num > 0)
                WakeUp();
        }

        Unlock();
    }

    void Stop()
    {
        Lock();
        // 如果有线程在休眠我要唤醒他们
        _is_running = false;
        WakeUpAll();

        Unlock();
    }

    ~ThreadPool()
    {
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
    }

private:
    int _thread_num;
    int _sleep_threads_num;

    bool _is_running;
    std::vector<Thread<F>> _threads; // 管理线程
    std::queue<F> _tasks_queue;      // 管理任务队列

    pthread_mutex_t _mutex; // 对队列资源的保护
    pthread_cond_t _cond;   // 条件变量看是否有任务
};