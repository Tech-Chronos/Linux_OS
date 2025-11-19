
<div align=center><img src="https://i-blog.csdnimg.cn/direct/cc2ada1a2c8a4452a1960b5949fe216b.png" width="250" height="250" alt="头像" /></div>

<div align=left><font face="华文楷体" size=4 color=#0000ff><b>🎉 博主首页：</b></font>
<a href="https://blog.csdn.net/m0_68617301"><font color=#9AC0CD><b>有趣的中国人</b></font></a>
<br /><br />
<font face="华文楷体" size=4 color=#0000ff><b>📚 C/C++ 专栏首页：</b></font>
<a href="https://blog.csdn.net/m0_68617301/category_12647125.html"><font color=#9AC0CD><b>【点我直达】C/C++ 进阶之路</b></font></a>

</div>

---

# C++ 右值引用、移动语义和完美转发：从 string 到 list 的完整理解

> 本文是我 C++ 专栏里“现代 C++ 语法”系列的一篇，主要围绕：**右值引用、移动构造、移动赋值、完美转发**，配合 `std::string` 和 `list::push_back` 这两个熟悉的例子，把几个容易混淆的概念串成一条线。

---

## 一、先把基础捋清楚：什么是左值？什么是右值？

很多人一上来就记“`T&&` 是右值引用”，但真正想明白移动语义，第一步应该是：**先区分左值 / 右值**。

### 1.1 直观记忆法

> **能取地址、能出现在赋值号左边的，一般就是左值；表达式计算出来的一次性结果，大多数是右值。**

```cpp
int x = 10;          // x 是左值
int y = x;           // x 是左值，y 是左值
int z = x + y;       // (x + y) 是右值 —— 运算的临时结果
```

再举几个典型的右值：

- 字面值：`10`、`3.14`、`"hello"`（注意：字符串字面量本身类型是 `const char[N]`，但通常也可以视为右值）
- 临时对象：`std::string("abc")`
- 函数返回的非引用对象：

```cpp
std::string MakeStr() {
    return std::string("hello"); // 返回的是临时对象
}

std::string s = MakeStr();       // MakeStr() 的结果是右值
```

### 1.2 学一点标准里的术语（不背也行）

C++11 把 “值类别” 细分成了三种：

- **prvalue（纯右值）**：`10`、`x + y`、`std::string("abc")` 这种“计算结果”。
- **xvalue（将亡值）**：已经“快被搬空”的对象，比如 `std::move(s)`。
- **lvalue（左值）**：能取地址、有持久名字的对象，比如变量 `s` 本身。

这三个名词不用死记，只要有个印象：**将亡值（xvalue）也是右值的一种**，它就是“可以安全偷资源的对象”。

---

## 二、右值引用：给右值取一个“名字”

### 2.1 左值引用 vs 右值引用

```cpp
int x = 10;
int&  lref = x;      // 左值引用，只能绑定左值
int&& rref = 10;     // 右值引用，只能绑定右值（字面值、临时对象等）
```

> **右值引用 = 给右值取一个“可操作的别名”**。

有了 `T&&`，我们就可以对临时对象做一些更高级的操作，比如：**偷资源、避免没必要的深拷贝**。

### 2.2 一个容易被忽略的细节：右值引用本身是“左值”

```cpp
int&& rr = 10;  // rr 是一个右值引用变量
rr = 20;        // 说明 rr 自身是左值（可以出现在赋值号左边）
```

所以：

- “被绑定的对象”是右值；
- 但“这个引用变量”依然是一个有名字的左值。

这件事后面讲完美转发时非常关键。

---

## 三、string 的移动构造：this->swap(str) 背后发生了什么？

假设我们写了这样两行代码：

```cpp
std::string ss  = std::string("111");
std::string str = std::string("222");
```

这里的 `std::string("111")` 和 `std::string("222")` 都是**临时对象（右值）**。为了减少没必要的拷贝，`std::string` 里实现了 **移动构造函数**，把临时对象里的资源直接“搬”到目标对象中。

我们不看标准库源码，自己写一个简化版 `String` 类来感受一下：

```cpp
class String
{
public:
    String(const char* s = "")
    {
        _size = strlen(s);
        _data = new char[_size + 1];
        memcpy(_data, s, _size + 1);
    }

    // 拷贝构造：深拷贝一份
    String(const String& other)
    {
        _size = other._size;
        _data = new char[_size + 1];
        memcpy(_data, other._data, _size + 1);
    }

    // 移动构造：偷指针
    String(String&& other) noexcept
    {
        _size = other._size;
        _data = other._data;

        other._size = 0;
        other._data = nullptr;
    }

    // 经典写法：用值传递 + swap 实现赋值运算符
    String& operator=(String rhs)
    {
        swap(rhs);  // this->swap(rhs);
        return *this;
    }

    void swap(String& other) noexcept
    {
        std::swap(_size, other._size);
        std::swap(_data, other._data);
    }

    ~String()
    {
        delete[] _data;
    }

private:
    size_t _size = 0;
    char*  _data = nullptr;
};
```

这里有几个关键点：

1. `String(String&& other)` 是**右值引用版本构造函数**，通常叫“移动构造函数”。  
2. 内部实现基本套路都是：**简单地偷指针 + 把源对象指针置空**。  
3. 赋值运算符用“值传递 + `swap`”这一经典写法：

   - 调用处：
     ```cpp
     String a("111");
     String b("222");
     b = a;                // 形参 rhs 由 a 拷贝构造
     b = String("tmp");    // 形参 rhs 由临时对象移动构造
     ```
   - 在 `operator=` 内部，我们把当前对象和形参 `rhs` 做一次 `swap`。  
   - 函数结束后，临时的 `rhs` 析构，顺带清理了旧资源。

> 🔑 结论：**右值引用让我们有机会写出移动构造 / 移动赋值，从而减少深拷贝带来的开销。**

---

## 四、list::push_back：为什么要写两个版本？

看一个更贴近日常写代码的例子：`std::list<std::string>::push_back`。

在没有移动语义之前，我们的链表 `push_back` 一般长这样：

```cpp
bool push_back(const std::string& s)
{
    Node* newnode = new Node(s);  // 一定会拷贝构造
    // ...
}
```

`Node` 的构造函数可能是：

```cpp
class Node
{
public:
    Node(const std::string& s)
        : _str(s)         // 拷贝构造
        , _next(nullptr)
        , _prev(nullptr)
    {}

private:
    std::string _str;
    Node* _next;
    Node* _prev;
};
```

现在如果我们写：

```cpp
std::list<std::string> lst;
lst.push_back(std::string("hello"));
```

`std::string("hello")` 是个右值，但我们 `push_back` 只有一个 `const std::string&` 版本，只能**退化成一次拷贝构造**。

### 4.1 加上右值版本的 push_back

更现代一点的实现会写成：

```cpp
bool push_back(const std::string& s)   // 1. 接左值
{
    Node* newnode = new Node(s);      // 拷贝构造
    // ...
}

bool push_back(std::string&& s)       // 2. 接右值
{
    Node* newnode = new Node(std::move(s));  // 触发移动构造
    // ...
}
```

对应地，`Node` 也要多加一个“移动构造版”的构造函数：

```cpp
class Node
{
public:
    Node(const std::string& s)
        : _str(s)
        , _next(nullptr)
        , _prev(nullptr)
    {}

    Node(std::string&& s)
        : _str(std::move(s))   // 把右值直接搬进来
        , _next(nullptr)
        , _prev(nullptr)
    {}

private:
    std::string _str;
    Node* _next;
    Node* _prev;
};
```

> ⭐ 核心思路：**从 list 的接口一路往里看，右值一路“绿色通道”到最底层 Node 的成员，能拷贝的地方尽量改成移动。**

如果你脑子里把这条链路想清楚了，那么：

- 为什么容器要重载 `push_back(const T&)` 和 `push_back(T&&)`？  
- 为什么成员里还要再写一遍 `Node(const T&)` 和 `Node(T&&)`？  

这些问题自然就都顺了。

---

## 五、万能引用与完美转发：右值引用的“高配用法”

前面讲的 `T&&` 都是**已知类型**的右值引用，比如 `std::string&&`。但在模板里，如果你写的是：

```cpp
template <class T>
void PerfectForward(T&& t)   // 注意：这里的 T 是待推导的
{
    // ...
}
```

这个 `T&&` 就不再是“普通右值引用”，而是所谓的：

> **万能引用（Forwarding Reference / 通用引用）**。

它有一个非常重要的特性：

- 如果你传进来的是左值，`T` 会被推导成 `T&`，于是形参类型是 `T& &&`，再折叠成 `T&`，也就是左值引用。
- 如果你传进来的是右值，`T` 就是普通的 `T`，形参类型就是 `T&&`。

用代码感受一下：

```cpp
std::string s = "hello";

PerfectForward(s);            // T 推导为 std::string&，形参类型：std::string&
PerfectForward(std::string("world"));
                              // T 推导为 std::string，形参类型：std::string&&
```

### 5.1 为什么说“右值引用本身是左值”很关键？

还记得我们前面说的吗：

> `T&& t` 这个形参本身有名字，所以它就是一个左值。

也就是说，在 `PerfectForward` 函数体内：

```cpp
template <class T>
void PerfectForward(T&& t)
{
    // t 在这里始终是一个“左值”！
}
```

无论你传进来的是左值还是右值，**`t` 自己永远是左值**。

这就引出一个问题：如果我要把参数继续往下传，并且希望：

- 传进来是左值，就继续当左值传；
- 传进来是右值，就继续当右值传；

该怎么办？

### 5.2 std::forward：完美转发的关键

标准库给了我们一个工具：`std::forward`。

```cpp
template <class T>
void PerfectForward(T&& t)
{
    // 保持“原样”转发
    callee(std::forward<T>(t));
}
```

解释一下这句关键代码：

- 当 `T` 是 `U&` 时，`std::forward<T>(t)` 返回左值；
- 当 `T` 是 `U` 时，`std::forward<T>(t)` 返回右值。

所以 `std::forward<T>(t)` 的效果就是：**把调用者传进来的“值类别”完整地保留下来**。

> 🔑 这就是“完美转发”的真正含义：**不改变原始实参是左值还是右值的属性。**

一个容器典型的用法：

```cpp
template <class T>
class MyList
{
public:
    template <class U>
    void push_back(U&& value)
    {
        // 这里的 _data.push_back 可以有左值版和右值版
        _data.push_back(std::forward<U>(value));
    }

private:
    std::list<T> _data;
};
```

这样写的好处是：

- 传左值时，调用 `push_back(const T&)`。  
- 传右值时，调用 `push_back(T&&)`。  

**一套接口，两个世界都照顾到了。**

---

## 六、整篇小结 & 记忆串联

最后用几句话把整篇文章串起来：

1. **左值 / 右值**：能取地址、有名字的是左值；字面量、表达式结果、临时对象是右值。  
2. **右值引用 `T&&`**：给右值取别名，便于实现移动语义。  
3. **移动构造 / 移动赋值**：本质套路都是“偷指针 + 清空源对象”。`this->swap(rhs)` 是一个优雅的常见写法。  
4. **容器中的右值优化**：
   - 从外层接口 `push_back(T&&)` 开始；
   - 一路往里写到 `Node(T&&)`；
   - 每一层都尽量用移动替代拷贝。  
5. **万能引用 `T&&` + `std::forward`**：
   - 模板里形如 `T&&` 的参数，可能是左值引用也可能是右值引用；  
   - `std::forward<T>(t)` 负责“完美转发”，保持参数原来的值类别。  

如果你能在脑子里面把这条链路想清楚：

> **右值 —— 右值引用 —— 移动构造 —— 容器 `push_back` —— 万能引用 —— 完美转发**

那么以后再看到各种各样的现代 C++ 接口，基本都能一眼看穿它们的用意。

---

<div align=center>
<font face="华文楷体" size=4 color=#FF8C00>
<b>✨ 如果这篇文章对你有一点点帮助，欢迎 点赞 / 收藏 / 关注专栏，都是我继续更新的动力！</b>
</font>
</div>
