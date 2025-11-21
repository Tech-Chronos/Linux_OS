
<div align=center><img src="https://i-blog.csdnimg.cn/direct/cc2ada1a2c8a4452a1960b5949fe216b.png" width="250" height="250" alt="头像" /></div>

<div align=left><font face="华文楷体" size=4 color=#0000ff><b>🎉博主首页：</b></font>
<a href="https://blog.csdn.net/m0_68617301?spm=1010.2135.3001.5343"><font color=#9AC0CD><b>有趣的中国人</b></font></a>
<br /><br />

<div align=left><font face="华文楷体" size=4 color=#0000ff><b>🎉专栏首页：</b></font>
<a href="https://blog.csdn.net/m0_68617301/category_12863943.html"><font color=#9AC0CD><b>操作系统原理</b></font></a>
<br /><br />

<div align=left><font face="华文楷体" size=4 color=#0000ff><b>🎉其它专栏：</b></font>
<a href="https://blog.csdn.net/m0_68617301/category_12602970.html?spm=1001.2014.3001.5482"><font color=#9AC0CD><b>C++初阶 |</b></font></a>
<a href="https://blog.csdn.net/m0_68617301/category_12647125.html?spm=1001.2014.3001.5482"><font color=#9AC0CD><b> C++进阶 |</b></font></a>
<a href="https://blog.csdn.net/m0_68617301/category_12594242.html?spm=1001.2014.3001.5482"><font color=#9AC0CD><b> 初阶数据结构</b></font></a>

<p align="center">
  <img src="https://img-blog.csdnimg.cn/direct/d05bcfa8df8549ba8c68dfabba4dd1c2.gif" alt="分割线动图" />
</p>

> <font color=black><b>这篇笔记整理的是 Linux 中文件与文件描述符 fd 的底层实现。</b></font>  
> <font color=#4169E1><b>open / read / write、struct file、fd 数组、VFS、多态的 file_operations</b></font> 都串在一起。  
> <font color=black><b>打开一个文件时内核到底做了什么，为什么 0/1/2 会默认连着终端，“一切皆文件”靠的是什么抽象，下面一口气说清楚。</b></font>  

@[toc]

---

# 1. 文件的两种视角

## 1.1 应用层视角：就是一个“能读写的东西”

在应用层，操作文件非常直接：

```c
int fd = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0664);
write(fd, "hello\n", 6);
close(fd);
```

程序员只关心几个动作：

- 打开：`open` 得到一个整数 fd；
- 读写：`read` / `write`；
- 关闭：`close`。

fd 在代码里看上去只是一个 int，背后却连着整个文件系统和 I/O 子系统。

## 1.2 内核视角：一套结构体 + 一条“路径”

从内核角度，完成一次读写大致要走这一条链路：

```text
用户空间:
    read(fd, buf, size)
        ↓  （陷入内核，系统调用入口）
内核:
    struct files_struct[struct file* fd_arr](fd) → struct file
        																			   → file_operations 里的 read 函数指针
        																			   → 具体文件系统或设备驱动
        																			   → 页缓存 / 缓冲区
        																         → 硬件（磁盘、终端、管道、socket ...）
```

关键抽象就是：

- `fd`：只是当前进程里的一个下标；
- `struct file`：描述“一个打开着的文件实例”的内核对象；
- `file_operations`：描述这类文件的具体操作方法。

---

# 2. open 的 flags 和 umask

## 2.1 常见 flags

`open` 第二个参数的常见取值：

- 访问模式（必选其一）：
  - `O_RDONLY`：只读；
  - `O_WRONLY`：只写；
  - `O_RDWR`：读写。

- 创建 & 覆盖相关：
  - `O_CREAT`：不存在就创建；
  - `O_EXCL`：和 `O_CREAT` 配合，若已存在则失败；
  - `O_TRUNC`：将已有文件长度截断为 0；
  - `O_APPEND`：每次写都追加到文件末尾。

示例：

```c
int fd = open("log.txt",
              O_WRONLY | O_CREAT | O_APPEND,
              0644);
```

第三个参数只有在 `O_CREAT` 时才有意义。

## 2.2 mode 与 umask

新建文件时，权限并不完全由 `mode` 决定，还会被进程的 `umask` 屏蔽：

```text
实际权限 = mode & ~umask
```

- `mode = 0644`
- 比如 `umask = 0002`
- 则最终权限为 `0644 & ~0002 = 0644 & 0775 = 0644`

shell 中可以用 `umask` 命令查看和修改。

---

# 3. 文件描述符 fd 的真正来源

## 3.1 每个进程有一个 fd 数组

在 `task_struct` 里，有一个指向 `files_struct` 的指针，里面维护着当前进程所有打开文件：

```c
struct files_struct {
    struct file *fd_array[NR_OPEN_DEFAULT]; // 下标就是 fd
    // 还有一些扩展表、锁等
};
```

所以：

> **fd 本质就是 `fd_array` 的下标**。

`fd_array[0]`、`fd_array[1]`、`fd_array[2]` 分别对应标准输入、输出、错误。

## 3.2 struct file：真正描述“打开的那个文件”

`struct file` 是内核层面的“打开文件对象”：

```c
struct file {
    struct file_operations *f_op; // 方法表
    void        *private_data;   // 设备或文件系统私有数据
    loff_t       f_pos;          // 当前读写偏移
    int          f_flags;        // 打开标志位（来自 open 的 flags）
    // 还有引用计数、挂接到 inode 的指针等
};
```

它和“磁盘上的那个文件实体”（inode）不是一回事：

- inode 描述的是“这个文件”本身（大小、权限、所在盘块等）；  
- `struct file` 描述的是“某个进程打开的一个实例”（偏移是多少，flags 是什么）。

---

# 4. open 调用时内核做了什么？

结合上面的结构，可以把 `open` 的主要步骤归纳成三步（概念层面）：

1. **创建并初始化 struct file 结构体**
   - 在内核的“打开文件表”中找一个空闲项；
   - 填写 `f_flags`、初始 `f_pos = 0` 等；
   - 通过路径解析找到目标 inode，并根据 inode 的类型，设置好 `f_op` 指针。
   
2. **把 struct file* 填入当前进程的 fd 数组**
   - 在 `fd_array` 中找一个空槽，比如下标 3；
   - `files->fd_array[3] = 新建的 struct file*`。
   
3. **返回这个下标作为 fd**
- 用户空间拿到的返回值就是整数 `3`。

以后每次 `read(3, ...)`，内核都会沿着这条链路走回去：

```text
3  → current->files->fd_array[3]  → struct file
   → struct file_operations->read → 具体实现
```

---

# 5. fd 0、1、2：标准输入输出从哪来？

几乎所有进程启动时都有 0、1、2 三个 fd 已经打开：

- `0`：标准输入（stdin）  
- `1`：标准输出（stdout）  
- `2`：标准错误（stderr）

它们的本质和其他 fd 一样，只是内核在创建进程时帮我们提前做好了几件事：

1. 为终端设备（或父进程重定向的目标）创建对应的 `struct file`；  
2. 把它们分别填到 `fd_array[0]`、`fd_array[1]`、`fd_array[2]`；  
3. 从此以后，用户态通过 `read(0, ...)`、`write(1, ...)` 等接口访问终端或文件。

例如，命令行里执行：

```bash
./a.out > out.log 2>&1
```

shell 在 `exec` 之前会做重定向：

- 把一个指向 `out.log` 的 `struct file*` 填到子进程的 `fd_array[1]` 和 `fd_array[2]`；
- 这样程序里对 `stdout` 和 `stderr` 的写入就全部落在 `out.log` 上了。

---

# 6. VFS 与 file_operations：多态的关键

Linux 的 VFS（Virtual File System）对上暴露统一的“文件”接口，对下对接各种文件系统和设备驱动。

核心思想：

- 抽象出通用的数据结构：`inode`、`dentry`、`file` 等；
- 抽象出一组操作函数指针：`struct file_operations`。

简化的 `file_operations`：

```c
struct file_operations {
    ssize_t (*read) (struct file *filp, char __user *buf,
                     size_t len, loff_t *pos);
    ssize_t (*write)(struct file *filp, const char __user *buf,
                     size_t len, loff_t *pos);
    loff_t  (*llseek)(struct file *filp, loff_t offset, int whence);
    int     (*open) (struct inode *inode, struct file *filp);
    int     (*release)(struct inode *inode, struct file *filp);
    // ...
};
```

不同类型的文件/设备，会有不同的实现：

- 普通磁盘文件：`ext4_file_operations`；  
- 字符设备（终端、串口）：`tty_fops`；  
- 管道：`pipefifo_fops`；  
- socket：`socket_file_ops`；  
- `/proc` 伪文件系统也有自己的 ops。

当用户调用 `read(fd, buf, len)` 时，内核大致这么走：

```c
sys_read(int fd, void __user *buf, size_t count)
{
    struct file *f = current->files->fd_array[fd];
    return f->f_op->read(f, buf, count, &f->f_pos);
}
```

这一句 `f->f_op->read(...)` 就是 C 语言手搓的“多态调用”：  
接口统一，但具体行为由不同的 `file_operations` 决定。

---

# 7. “一切皆文件”的真正含义

从用户态 API 来看，无论是：

- 普通文件；
- 目录（`open` 后用 `getdents` 读取）；
- 终端；
- 管道；
- socket；
- `/proc`、`/sys` 这类伪文件系统；

本质上都是：

1. 用 `open`（或 `socket` 之类的特定系统调用）拿到一个 fd；  
2. 通过 `read` / `write` / `ioctl` 等接口进行操作。

在内核内部：

- 它们都对应某种 `struct file`；  
- 都有各自的 `file_operations`；  
- 都挂在 VFS 抽象之下。

所以那句 “Everything is a file” 并不是说所有东西都真的存成磁盘文件，而是说：

> **在接口层面，内核用“文件”这一套抽象来统一地管理各种对象。**

---

# 8. 语言层的再次封装

C 语言标准库的 `FILE*`，C++ 的 `fstream`、`ifstream`、`ofstream` 等，其实都是在 fd 外面再包了一层：

- 管理用户态缓冲区，减少系统调用次数；  
- 提供格式化读写接口（`printf`、流插入运算符 `<<` 等）；  
- 附带异常处理、RAII 等高级特性。

底层还是那几个系统调用：

```c
open / read / write / lseek / close
```

理解了 fd 和 `struct file` 这一层之后，再往上看各种封装就比较自然了：  
它们不是“新东西”，只是对已有机制的包装和增强。

---

# 9. 小结

- 文件描述符 fd 是当前进程 `fd_array` 的一个下标，指向内核里的 `struct file`；  
- `struct file` 描述的是“某个打开实例”，包含偏移、flags、方法表指针等信息；  
- 0、1、2 在进程创建时被内核自动填好，分别对应标准输入、标准输出、标准错误；  
- VFS 利用 `file_operations` 实现了 C 语言版多态，让各种文件系统和设备在统一接口下共存；  
- 上层语言对文件的各种封装，本质都是围绕底层 fd 和这些系统调用展开的。

把这几层结构在脑子里连起来，读写文件时就不会只想到一个“int fd”，而是能顺着链路想到后面那整套文件系统和驱动。
