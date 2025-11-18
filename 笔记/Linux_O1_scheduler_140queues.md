
<div align=center><img src="https://i-blog.csdnimg.cn/direct/cc2ada1a2c8a4452a1960b5949fe216b.png" width="250" height="250" alt="头像" /></div>

<div align=left><font face="华文楷体" size=4 color=#0000ff><b>🎉博主首页：</b></font>
<a href="https://blog.csdn.net/m0_68617301?spm=1010.2135.3001.5343"><font color=#9AC0CD><b>有趣的中国人</b></font></a>
<br /><br />

<div align=left><font face="华文楷体" size=4 color=#0000ff><b>🎉专栏首页：</b></font>
<a href="https://blog.csdn.net/m0_68617301/category_12652366.html?spm=1001.2014.3001.5482"><font color=#9AC0CD><b>操作系统原理</b></font></a>
<br /><br />
<div align=left><font face="华文楷体" size=4 color=#0000ff><b>🎉其它专栏：</b></font>
<a href="https://blog.csdn.net/m0_68617301/category_12602970.html?spm=1001.2014.3001.5482"><font color=#9AC0CD><b>C++初阶 |</b></font></a>
<a href="https://blog.csdn.net/m0_68617301/category_12647125.html?spm=1001.2014.3001.5482"><font color=#9AC0CD><b> C++进阶 |</b></font></a>
<a href="https://blog.csdn.net/m0_68617301/category_12594242.html?spm=1001.2014.3001.5482"><font color=#9AC0CD><b> 初阶数据结构</b></font></a>


<p align="center">
  <img src="https://img-blog.csdnimg.cn/direct/d05bcfa8df8549ba8c68dfabba4dd1c2.gif" alt="分割线动图" />
</p>

> <font color=black>**这一篇，我们把你笔记里关于 „140 队列调度“ 的内容整理成一篇真正能看懂、能讲出来的文章。**</font>  
> <font color=#4169E1>Linux O(1) 调度器、140 个优先级队列、bitmap、runqueue、active/expired 双数组</font> 全部串起来。  
> <font color=black><b>看完这篇，你可以对着纸画出调度结构，并把调度过程流利讲给别人听。</b></font>  

@[toc]

---

# 1. 背景：Linux 为什么要搞一个 “O(1) 调度器”？

在早期的 Linux 内核版本中，调度器比较朴素：  
- 所有可运行进程放在一个队列中；  
- 每次调度要在队列里扫描一圈，选出最合适的进程；  
- 复杂度接近 O(n)，进程一多，调度开销明显。

为了解决 **进程数多时调度效率掉下去** 的问题，Linux 2.6 之前引入了著名的 **O(1) 调度器**：

- 不管系统里有 10 个进程还是 1000 个进程，**挑选下一个要运行的进程所花的时间近似常数级**（O(1)）；  
- 关键思路就是：  
  1. 把优先级离散化成固定数量（140 个）；  
  2. 每个优先级维护自己的队列；  
  3. 使用 **bitmap** 快速定位到“当前有任务的最高优先级”。

> 现在新内核用的是 CFS（完全公平调度器），但 O(1) 调度器仍然非常适合作为“调度算法入门模型”。

---

# 2. 140 个优先级是怎么来的？

Linux 把优先级分成两大类：

1. **实时进程（Real-time）**  
   - 静态优先级范围：`0 ~ 99`  
   - 数值越小优先级越高（0 是最高）  

2. **普通进程（Normal）**  
   - 静态优先级范围：`100 ~ 139`  
   - 又由 `nice` 值映射而来（`-20 ~ +19`）  

合在一起，一共就是：  

> `0 ~ 139` 共 **140 个优先级**。

在 O(1) 调度器里，每一个优先级都对应一个队列，里面是拥有相同优先级的 `task_struct`。

---

# 3. runqueue 与 prio_array：调度器的“骨架”

在 O(1) 调度器中，每个 CPU 有一个 **运行队列 runqueue**，内部用两个数组结构来管理可运行进程：

```c
struct prio_array {
    unsigned int nr_active;               // 当前有多少进程在这个数组
    unsigned long bitmap[BITMAP_SIZE];    // 哪些优先级上有任务
    struct list_head queue[NR_PRIO];      // 每个优先级对应一个双向链表
};

struct runqueue {
    struct prio_array *active;   // 正在使用的就绪队列
    struct prio_array *expired;  // 时间片用完、等待“轮回”的队列
    // ... 其他字段：正在运行的进程、负载统计等等
};
```

你笔记里画的简化版类似于：

```c
struct q {
    long bitmap[?];              // 一个优先级一个 bit
    task_struct *queue[140];     // 每个优先级都有一个队列头指针
};
// runqueue 里有 struct q q[2];  // active / expired 两份
```

### 小结

- **`queue[prio]`**：存放所有这个优先级的 task_struct，采用链表（FIFO，近似轮转调度）；  
- **`bitmap`**：快速标记“哪些优先级的队列非空”；  
- **`nr_active`**：当前数组下总共有多少就绪进程。

---

# 4. bitmap：如何 O(1) 找到最高优先级？

核心问题：**如何在 140 个优先级里，快速找到“当前有任务的最高优先级”？**

有两个关键点：

1. 140 个优先级是固定的，可以预先决定使用多少个 `unsigned long` 作为位图：
   - 例如 32 位系统下，一个 `unsigned long` 有 32 bit  
   - 只要 `ceil(140 / 32) = 5` 个 long 就够了（你笔记里算的 4×32=128，其实一般会按 5 个 long 布置）

2. 每个优先级对应一个 bit：  
   - 当 `queue[prio]` 非空，就把对应 bit 置 1；  
   - 为空时，清 0。

查找“最高优先级的可运行进程”时：

1. 在 bitmap 里找 **最低的那个置 1 的 bit**（因为 0 是最高优先级）；
2. 再用这个优先级 index 在 `queue[prio]` 中取出队列头即可。

```c
int highest_prio(struct prio_array *array) {
    int group = find_first_bit(array->bitmap);   // 先找到第一个非空 group
    int prio  = group * BITS_PER_LONG
              + find_first_bit(&array->bitmap[group]);
    return prio;
}
```

> 核心：priority 总数是固定的，而且 bitmap 大小也固定。每次调度都是在这几个值上做位运算，复杂度近似 O(1)。

---

# 5. active / expired：为什么要两份 prio_array？

你在笔记里专门写了一句：

> 在 runqueue 中，存在两个指针 `struct q* active` 和 `struct q* expired`。  
> active 只包含“还没用完时间片的 PCB”，expired 只包含“已经用完时间片的 PCB”。

这其实是 O(1) 调度器中最有意思的一点。

## 5.1 直观理解

- **active**：当前这一轮要调度的就绪进程。  
- **expired**：时间片用完、需要“等下一轮”的进程。

调度流程简单描述：

1. 从 `active` 中选出一个最高优先级的进程 A，运行它；  
2. A 的时间片减少；如果没用完，再放回 `active` 的对应优先级队列尾部；  
3. 如果时间片用完，就根据动态优先级重新计算它的优先级，然后把它 **塞进 `expired` 对应队列**；  
4. 当 `active->nr_active == 0`，说明本轮所有进程时间片都跑光了：  
   - 交换指针：`swap(active, expired)`  
   - 又开始新一轮调度。

> 重要的是：**PCB 不会被复制**，只是从一个 prio_array 的队列链接到另一个 prio_array 的队列中，指针在移动而已。

## 5.2 为什么要这样设计？

- 避免一个饥饿进程一直排在末尾。每轮结束都会重新计算优先级，并把“老实进程”挪到更高的优先级；  
- 两份 prio_array 切换非常快，只是两个指针互换：O(1)。

---

# 6. 具体调度过程：一步一步走

我们用一个简化过程串一下（只看普通进程）：

1. **新进程创建**  
   - 根据 `nice` 值计算静态优先级；  
   - 插入到 `active->queue[prio]` 的队尾；  
   - 把 bitmap 对应 bit 置 1。

2. **schedule() 被调用**（时钟中断 / 主动让出 CPU 等）：  
   1. 在 `active->bitmap` 中找到最高优先级 `prio`；  
   2. 取出 `queue[prio]` 的队头 task_struct，设为 `current`；  
   3. 切换到它的内核栈和上下文，重新回到用户态执行。

3. **时间片耗尽**  
   - 时钟中断发现该进程时间片用完；  
   - 重新计算它的“动态优先级”（考虑交互性、睡眠时间等）；  
   - 把它插入 `expired->queue[new_prio]` 队尾；  
   - 如果原优先级队列空了，就清除 bitmap 的对应 bit。

4. **active 空了怎么办？**  
   - 当 `active->nr_active == 0`：  
     ```c
     struct prio_array *tmp = active;
     active  = expired;
     expired = tmp;
     ```
   - 所有跑完一轮时间片的进程，又重新进入下一轮竞争。

---

# 7. 用一个小例子直观感受一下

假设系统里只有 4 个普通进程（为了简单，忽略实时进程）：

| 进程 | nice | 静态优先级（假设） |
|------|------|-------------------|
| A    | 0    | 120               |
| B    | 5    | 125               |
| C    | -5   | 115               |
| D    | 10   | 130               |

### 1）初始化

- 4 个进程都在 `active` 中：  
  - `queue[115]`：C  
  - `queue[120]`：A  
  - `queue[125]`：B  
  - `queue[130]`：D  

bitmap 中对应的 4 个 bit 被置 1。

### 2）第一轮

调度顺序大致是（只看优先级，不考虑动态调整）：

1. 先选 115 → C 跑，时间片用完后进入 `expired`；  
2. 再选 120 → A 跑，用完后入 `expired`；  
3. 再选 125 → B；  
4. 再选 130 → D；  
5. 这时 `active` 空了，交换 active/expired，开始新一轮。

如果在过程中 C 长时间睡眠，又可以被动态加分，放到更高优先级（更靠前的队列），从而表现出更好的交互性。

---

# 8. 一些小问题

> ❓ **“数组中有很多空位（queue[prio] == NULL）怎么办，要遍历数组吗？”**  

不需要扫描整个数组，只需要依靠 **bitmap**：  
- 有任务的优先级才会在 bitmap 中设置为 1；  
- 查找时只扫描 bitmap，复杂度与“优先级总数”有关，而不是“进程总数”。

> ❓ **“为什么一定要 140 级，而不是随便 100 级？”**  

- Linux 要兼容 POSIX 实时优先级（0~99）；  
- 还要为普通进程留出 nice 映射空间，最终设计成 100~139；  
- 0~139 刚好 140 个，方便用固定大小的数据结构实现 O(1)。

> ❓ **“active / expired 之间会不会拷贝 task_struct？”**  

- 不会拷贝 PCB（task_struct）；  
- 只是把 task_struct 从一个队列的链表头/尾摘下来，挂到另外一个队列里；  
- 真正切换只是 `active` 和 `expired` 两个指针互换，非常快。

---

# 9. 总结：一口气把 O(1) 调度器复盘一遍

- Linux 早期 O(1) 调度器把优先级离散为 **140 级**；  
- 通过 `struct prio_array` + `bitmap + queue[prio]` 来组织可运行进程；  
- 依靠 **bitmap** 在 O(1) 时间内找到“最高优先级且有任务”的队列；  
- 使用 **active/expired 双数组** + 时间片，用简单的机制实现了动态优先级和基本公平；  
- PCB 只是在线性时间片轮转中在不同队列间移动，不做大规模复制。

> 当你能在纸上画出：**runqueue → prio_array → bitmap → queue → task_struct → active/expired** 这条链路，你对 Linux 进程调度的理解就已经很到位了。  

如果你之后想写一篇对比文章，比如：  
- O(1) 调度器 vs CFS 调度器；  
- 实时调度策略 `SCHED_FIFO` / `SCHED_RR`；  
- 如何在代码里用 `sched_setscheduler` 改变进程调度策略；  

