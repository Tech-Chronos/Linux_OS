#include <iostream>
#include <vector>
#include <memory>

using namespace std;

// void func()
// {
//     double i = 90.0;
//     int j = static_cast<int>(i);

//     cout << "j: " << j << endl;
//     cout << "i: " << i << endl;
// }
// class Base
// {
// public:
//     virtual ~Base()
//     {
//         //cout << "~Base()" << endl;
//     }

//     virtual void show()
//     {
//         cout << "Base:show()" << endl;
//     }
// private:
//     int _base_var;
// };

// class Derived : public Base
// {
// public:

//     virtual void show()
//     {
//         cout << "Derived:show()" << endl;
//     }

// private:
//     int _derived_var;
// };

// void func()
// {
//     Derived d1;

//     // Base* base_ptr = &d;

//     // base_ptr->show();

//     Base* base_ptr = static_cast<Base*>(&d1);

//     base_ptr->show();

//     Base b1;

//     Derived* der_ptr = dynamic_cast<Derived*>(&b1);

//     if (der_ptr == nullptr)
//     {
//         cout << "转换失败 " << endl;
//     }

// }

class Animal
{
public:
    virtual ~Animal() = default;
    virtual void speak() const = 0;
};

class Dog : public Animal
{
public:
    void speak() const override
    {
        std::cout << "Woof! Woof!" << std::endl;
    }

    void fetch() const
    {
        std::cout << "Fetching the ball..." << std::endl;
    }
};

class Cat : public Animal
{
public:
    void speak() const override
    {
        std::cout << "Meow~" << std::endl;
    }

    void climb() const
    {
        std::cout << "Climbing the tree..." << std::endl;
    }
};

void pointer_dynamic_cast_example()
{
    std::cout << "=== 指针类型的dynamic_cast示例 ===" << std::endl;

    // 场景1：安全的向下转换
    Animal *animal1 = new Dog(); // 基类指针指向派生类对象
    Dog *dog = dynamic_cast<Dog *>(animal1);

    if (dog != nullptr)
    {
        std::cout << "转换成功！" << std::endl;
        dog->speak();
        dog->fetch();
    }
    else
    {
        std::cout << "转换失败：不是Dog类型" << std::endl;
    }

    // 场景2：转换失败的情况
    Animal *animal2 = new Cat();
    Dog *not_dog = dynamic_cast<Dog *>(animal2);

    if (not_dog)
    {
        not_dog->fetch(); // 这行不会执行
    }
    else
    {
        std::cout << "转换失败：animal2不是Dog类型" << std::endl;
    }

    delete animal1;
    delete animal2;
}

void polymorphic_container_example()
{
    vector<unique_ptr<Animal>> animals;

    animals.emplace_back(make_unique<Dog>());
    animals.emplace_back(make_unique<Dog>());
    animals.emplace_back(make_unique<Cat>());

    int dog_count = 0;
    int cat_count = 0;

    for (const auto &animal : animals)
    {
        if (Dog *dog = static_cast<Dog *>(animal.get()))
        {
            cout << "this is a dog" << endl;
            dog->speak();
            dog->fetch();
            ++dog_count;
        }
        else if (Cat *cat = dynamic_cast<Cat *>(animal.get()))
        {
            cout << "this is a cat" << endl;
            cat->speak();
            cat->climb();
            ++cat_count;
        }
        else
        {
            cout << "unknown animal type!" << endl;
        }
    }

    cout << "dog_count: " << dog_count << "cat_count: " << cat_count << endl;
}

void compare_casts()
{
    std::cout << "\n=== dynamic_cast vs static_cast 对比 ===" << std::endl;

    // 创建基础对象
    Animal *animal_ptr = new Cat();

    // 使用dynamic_cast（安全）
    Dog *safe_dog = dynamic_cast<Dog *>(animal_ptr);
    if (safe_dog)
    {
        safe_dog->fetch();
    }
    else
    {
        std::cout << "dynamic_cast: 安全地检测到转换失败" << std::endl;
    }

    // 使用static_cast（不安全！）
    Dog *unsafe_dog = static_cast<Dog *>(animal_ptr);
    // unsafe_dog->fetch();  // 危险！未定义行为，可能导致程序崩溃

    std::cout << "static_cast转换完成，但可能不安全" << std::endl;

    delete animal_ptr;
}

void func()
{
    // const char* str = "hello";
    // char* ch_ptr = const_cast<char*>(str);

    // ch_ptr[0] = 'k';

    // cout << ch_ptr << endl;

    // cout << str << endl;

    const int i = 10;

    int *ptr = const_cast<int *>(&i);

    *ptr = 90;

    cout << *ptr << endl;

    char ch = 'a';

    double d = static_cast<double>(ch);
}

void function_pointer_conversion()
{
    std::cout << "\n=== 函数指针转换 ===" << std::endl;

    // 普通函数
    void (*normal_func)() = []()
    {
        std::cout << "普通函数被调用" << std::endl;
    };

    // 转换为void*（在某些API中需要）
    void *void_func_ptr = reinterpret_cast<void *>(normal_func);
    
    std::cout << "函数指针: " << normal_func << std::endl;
    std::cout << "转换为void*: " << void_func_ptr << std::endl;
    std::cout << sizeof(normal_func) << std::endl;
    std::cout << sizeof(void_func_ptr) << std::endl;

    // 恢复函数指针
    void (*restored_func)() = reinterpret_cast<void (*)()>(void_func_ptr);
    restored_func(); // 调用恢复的函数
}

int main()
{
    // pointer_dynamic_cast_example();
    // polymorphic_container_example();
    // compare_casts();

    //func();
    function_pointer_conversion();
    return 0;
}