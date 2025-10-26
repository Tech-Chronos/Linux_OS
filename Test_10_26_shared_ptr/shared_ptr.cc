#include <iostream>
#include <thread>
#include <list>
#include <mutex>

using namespace std;

mutex mtx;

namespace dsj
{
    // 引用计数
    template <class T>
    class my_shared_ptr
    {
    public:
        my_shared_ptr(T* ptr)
            :_ptr(ptr)
            ,_ref_cnt(new int(1))
            ,_mtx(new mutex)
        {}

        my_shared_ptr(const my_shared_ptr& my_shared_ptr)
        {
            _ptr = my_shared_ptr._ptr;
            _ref_cnt = my_shared_ptr._ref_cnt;
            _mtx = my_shared_ptr._mtx;
            _mtx->lock();
            ++(*_ref_cnt);
            _mtx->unlock();
        }

        // p1 = p2
        my_shared_ptr& operator=(const my_shared_ptr& ptr)
        {
            if (this != &ptr)
            {
                release();

                _ptr = ptr._ptr;

                _ref_cnt = ptr._ref_cnt;

                _mtx = ptr->_mtx;

                ++(*_ref_cnt);
            }

            return *this;
        }

        ~my_shared_ptr()
        {
            release();
        }

        size_t size()
        {
            return *_ref_cnt;
        }

        T* operator->()
        {
            return _ptr;
        }

        void release()
        {
            _mtx->lock();
            if (*_ref_cnt > 1)
            {
                --(*_ref_cnt);
            }
            else
            {
                delete _ptr;
                delete _ref_cnt;
            }
            _mtx->unlock();
        }
    private:
        T* _ptr;
        int* _ref_cnt;
        mutex* _mtx;
    };
}

void func(dsj::my_shared_ptr<std::list<int>> sptr, int n)
{
    for (int i = 0; i < n; ++i)
    {
        dsj::my_shared_ptr<std::list<int>> sptr1(sptr);
        dsj::my_shared_ptr<std::list<int>> sptr2(sptr);
        dsj::my_shared_ptr<std::list<int>> sptr3(sptr);

        mtx.lock();
        sptr->push_back(i);
        mtx.unlock();
    }
    std::cout << sptr.size() << std::endl;
}

int main()
{
    dsj::my_shared_ptr<std::list<int>> sptr(new std::list<int>);
    std::thread t1(func, sptr, 10000);
    std::thread t2(func, sptr, 20000);

    t1.join();
    t2.join();
    std::cout << "链表size：" << sptr->size() << std::endl;
    std::cout << sptr.size() << std::endl;

    return 0;
}