#include <iostream>

using namespace std;

// 饿汉模式

class single_instance_hungry
{
public:
    static single_instance_hungry *CreateSingleInstance()
    {
        return _single;
    }

private:
    single_instance_hungry();

    single_instance_hungry(const single_instance_hungry &) = delete;

    single_instance_hungry& operator=(const single_instance_hungry &) = delete;

private:
    static single_instance_hungry *_single;
};

single_instance_hungry *single_instance_hungry::_single = new single_instance_hungry;

// 懒汉模式
class single_instance_lazy1
{
public:
    static single_instance_lazy1& CreateSingleInstance()
    {
        static single_instance_lazy1 _single;

        return _single;
    }

private:
    single_instance_lazy1();

    single_instance_lazy1(const single_instance_lazy1 &) = delete;

    single_instance_lazy1& operator=(const single_instance_lazy1 &) = delete;
};

class single_instance_lazy2
{
public:
    static single_instance_lazy2* CreateSingleInstance()
    {
        if (_single == nullptr)
        {
            if (_single == nullptr)
            {
                _single = new single_instance_lazy2;
            }
        }
        return _single;
    }

private:
    single_instance_lazy2();
    single_instance_lazy2(const single_instance_lazy2 &) = delete;
    single_instance_lazy2& operator=(const single_instance_lazy2 &) = delete;

private:
    static single_instance_lazy2 *_single;
};

single_instance_lazy2 *single_instance_lazy2::_single = nullptr;