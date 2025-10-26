#include <iostream>
#include <thread>
#include <mutex>

using namespace std;


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

int main()
{
    dsj::my_shared_ptr<int> ptr(new int);
    return 0;
}