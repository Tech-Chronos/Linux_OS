
<div align=center><img src="https://i-blog.csdnimg.cn/direct/cc2ada1a2c8a4452a1960b5949fe216b.png" width="250" height="250" alt="头像" /></div>

<div align=left><font face="华文楷体" size=4 color=#0000ff><b>🎉 博主首页：</b></font>
<a href="https://blog.csdn.net/m0_68617301"><font color=#9AC0CD><b>有趣的中国人</b></font></a>
<br /><br />

<font face="华文楷体" size=4 color=#0000ff><b>📚 C/C++ 专栏首页：</b></font>
<a href="https://blog.csdn.net/m0_68617301/category_12647125.html"><font color=#9AC0CD><b>【点我直达】C/C++ 进阶之路</b></font></a>
</div>

---

# C++11 Lambda 表达式、堆上对象与可变参数

> 这篇算是我自己 10 月中那几页手写笔记的“电子版”，把当时记的几个零散知识点——**Lambda 表达式、本地只在堆上的类、可变参数模板、`emplace_back`**——整理成一篇能回头翻的博客。

---

## 一、Lambda 表达式的本质：一个匿名的函数对象

C++ 里 Lambda 的完整语法大概是这样：

```cpp
[capture](参数列表) mutable noexcept -> 返回类型 
{
    // 函数体
};
```

日常写的时候一般都省略一大堆：

```cpp
auto add = [](int a, int b) { return a + b; };
```

**底层本质**：编译器会为这个 Lambda 生成一个**匿名类**，大致等价于：

```cpp
struct __LambdaAdd
{
    int operator()(int a, int b) const
    {
        return a + b;
    }
};

__LambdaAdd add;
```

所以：

- Lambda 不是魔法，就是一个**仿函数对象（函数对象）**的语法糖；
- `[]` 里写的是“捕捉列表”，用来说明**这个匿名类要持有哪些外部变量**。

---

## 二、捕捉列表与 `mutable`：按值 vs 按引用

### 2.1 按值捕捉：默认带 `const` 属性

```cpp
int x = 10;

auto f = [x]() {
    // x 在这里是按值捕捉到闭包对象里的一个成员
    // operator() 默认是 const 成员函数
    // x 不能被修改
};
```

> 笔记里的话：`[] 捕捉列表 -> 有 const 属性`。

换成人话：

- `x` 被“拷贝一份”进来；
- Lambda 的 `operator()` 默认是 `const`，所以里面看到的那份 `x` 也是只读的。

如果硬要改怎么办？在参数列表后面加一个 **`mutable`**：

```cpp
int x = 10;

auto f = [x]() mutable {
    x++;     // ✅ 可以改，改的是捕捉进来的那一份拷贝
};

f();
// 外面的 x 依然是 10
```

> `mutable` 的含义：**允许在 `operator()` 中修改“按值捕获的成员变量”**。  
> 注意：它改的是自己的那份副本，**不会影响外面的变量**。

### 2.2 按引用捕捉：&[] 没有 const 限制

```cpp
int x = 10;

auto f = [&x]() {
    x++;     // 直接改外面的 x
};
```

- `&x` 把外面的 `x` 以引用方式捕捉进来；
- 对应到匿名类里，就是“保存一个 `int&` 成员”，`operator()` 里对它的修改会直接反映到外部。

常见写法还有：

```cpp
auto f1 = [=]() { /* 按值捕捉所有在作用域内用到的变量 */ };
auto f2 = [&]() { /* 按引用捕捉所有在作用域内用到的变量 */ };
```

我在笔记里特别划了两句：

- `[=]` —— **按值捕捉 all，有 const 属性**；
- `[&]` —— **按引用捕捉 all，无 const 属性**。

背下来之后，看到别人代码时脑子里直接映射到“是不是会改外面的变量”就行。

### 2.3 混合捕捉

有时候我们既想按值，又想按引用：

```cpp
int x = 1;
int y = 2;

auto f = [=, &y]() mutable {
    // x 按值捕捉，可以在 mutable 下改自己的副本
    // y 按引用捕捉，直接改外面的 y
    x++;
    y++;
};
```

这种写法在 STL 算法里会比较常见。

---

## 三、Lambda 为什么这么重要？——和标准库绑在一起

C++11 之后，标准库大量算法都鼓励你传一个“**可调用对象**”进去：

- 函数指针
- 仿函数（重载了 `operator()` 的类）
- Lambda 表达式

用 Lambda，可以直接把逻辑写在调用处，更贴近“就地写匿名回调”的感觉：

```cpp
std::vector<int> v{3, 1, 4, 1, 5, 9};

std::sort(v.begin(), v.end(), [](int a, int b) {
    return a > b;   // 降序排序
});
```

背后的思想是：**多态 + 模板 + 函数对象** 的组合，把“行为”也作为参数传来传去，标准库只负责“算法骨架”。

---

## 四、只能在堆上构造的类：HeapOnly 模式

有时候我们会希望一个类“**禁止在栈上创建，只能在堆上 new**”，比如：

- 构造和析构非常重，不希望函数一返回就自动销毁；
- 资源生命周期完全由工厂函数控制。

一个经典写法（对应我笔记里的 `HeapOnly` 图）：

```cpp
class HeapOnly
{
public:
    static HeapOnly* CreateObj()
    {
        return new HeapOnly();
    }

    // 禁用拷贝 & 赋值
    HeapOnly(const HeapOnly&)            = delete;
    HeapOnly& operator=(const HeapOnly&) = delete;

private:
    HeapOnly()
        : _a(1)
    {}

    ~HeapOnly() = default;

private:
    int _a;
};
```

使用方式：

```cpp
HeapOnly* p = HeapOnly::CreateObj();
// ...
delete p;   // 一定要记得 delete
```

关键点：

1. 构造函数设为 `private`，外部不能直接 `HeapOnly ho;`。  
2. 暴露一个 `static` 工厂函数 `CreateObj`，内部用 `new`。  
3. 拷贝构造、拷贝赋值全都 `= delete`，避免随便复制。  

> 真实项目里更推荐配合 `std::unique_ptr` 使用：

```cpp
std::unique_ptr<HeapOnly> ptr(HeapOnly::CreateObj());
```

或者干脆写一个专门的 `Create` 返回 `std::unique_ptr<HeapOnly>`，就不会忘记 `delete`。

---

## 五、可变参数模板：从 `printf` 风格到现代写法

C 风格的可变参数函数是这样：

```cpp
int printf(const char* fmt, ...);
```

C++11 提供了**可变参数模板（Variadic Template）**，可以做到“参数个数任意、多态”的效果。

### 5.1 一个简单的递归版本

```cpp
void my_print()
{
    std::cout << std::endl;
}

template <class T, class... Args>
void my_print(T value, Args... args)
{
    std::cout << value << " ";
    my_print(args...);   // 递归展开参数包
}
```

使用：

```cpp
my_print(1, 2.5, "hello", std::string("world"));
```

模板参数包 `Args...` + 函数参数包 `args...` 这套组合，就是我笔记里的那句：

> `template<class T, class... Args> void Afg_print(Args... args)`。

### 5.2 C++17：折叠表达式的更优雅写法

如果允许用到 C++17，可以写得更短：

```cpp
template <class... Args>
void my_print2(Args&&... args)
{
    (std::cout << ... << args) << std::endl;   // 左折叠表达式
}
```

这其实是在做：`((std::cout << arg1) << arg2) << arg3 ...`。

再配合前一篇里讲到的“完美转发”，还可以写成：

```cpp
template <class... Args>
void my_print3(Args&&... args)
{
    (std::cout << ... << std::forward<Args>(args)) << std::endl;
}
```

---

## 六、`emplace_back` vs `push_back`：少一次临时对象

最后一个点是容器的 `emplace_back`，典型场景就是插入 `pair`：

```cpp
std::vector<std::pair<std::string, int>> v;
```

### 6.1 `push_back`：先构造临时对象，再拷贝 / 移动

```cpp
v.push_back(std::make_pair(std::string("Tom"), 18));
```

这里的过程大致是：

1. `std::make_pair` 在外面构造出一个临时 `pair<std::string, int>`；
2. 再把这个临时对象拷贝 / 移动进 `vector` 里的那块内存。

### 6.2 `emplace_back`：原地构造（就地 new）

```cpp
v.emplace_back("Tom", 18);
```

- 它把参数 `"Tom"` 和 `18` 直接**转发到 `pair` 的构造函数**，在 `vector` 内部那块内存上原地构造对象；
- 中间不会产生多余的临时 `pair`，也就不会有多余的拷贝 / 移动。

对于 `list<std::string>` 之类的容器：

```cpp
std::list<std::string> lst;

lst.push_back(std::string("hello"));  // 构造临时 string，再拷贝/移动到节点里
lst.emplace_back("world");            // 直接在节点里用 "world" 构造 string
```

> 在我自己的理解里，可以这样记：  
>
> - `push_back(x)`：**“我已经有一个对象 x，你帮我塞进去”**；  
> - `emplace_back(args...)`：**“你帮我在里面 new 一个对象，构造参数是 args...”**。  

当插入的是比较重的对象（`std::string`、自定义类、`pair` 等等），`emplace_back` 能少一次临时对象构造，配合右值引用 / 完美转发，就挺香的。

---

## 七、最后的小结

这篇笔记型博客，把几块常在一起出现的 C++11 新特性串了下：

1. **Lambda 表达式**：匿名函数对象，本质是编译器帮你生成的带 `operator()` 的类。  
2. **捕捉列表 + `mutable`**：按值捕捉默认 `const`，`mutable` 能改自己的副本；按引用捕捉可以直接改外部变量。  
3. **只能在堆上的类**：`private` 构造 + `static` 工厂 + `= delete` 拷贝构造/赋值。  
4. **可变参数模板**：`Args...` 和 `args...` 这对“参数包 + 展开”，可以优雅地实现任意参数个数的函数。  
5. **`emplace_back`**：直接把构造参数传给容器内部对象，在容器内原地构造，避免多余临时对象。  

以后哪天再翻回这篇的时候，希望还能一眼想起那本笔记本上红笔、黑笔混写的那几页。

---

<div align=center>
<font face="华文楷体" size=4 color=#FF8C00>
<b>✨ 如果这篇文章对你有一点点帮助，欢迎 点赞 / 收藏 / 关注专栏，都是我继续更新的动力！</b>
</font>
</div>
