# Bonus

括号里的分值为最高得分（分数为期末总分），最终得分由 code review 确定。另请注意，总分值不能超过 1 分。

## B+ 树相关

### 缓存 *(0.2%)*

如果将一些常用数据存在内存中，可以省去大量从外存上读取的时间，这就是缓存的意义。

此 bonus 根据实现难度和工作量给分。推荐使用 [LRU 算法](https://en.wikipedia.org/wiki/Cache_replacement_policies#Least_recently_used_(LRU))。

关于正确性，你需要用开启缓存的程序通过 OJ 测试。

**注意:** 不允许把全部数据储存在内存中。

### 空间回收 *(0.1%)*

在删除 B+ 树中某些数据后，可能会产生一些空间碎片。此 bonus 要求实现空间回收功能，使得删除数据后，B+ 树可以在下次需要空间时利用这些空间。

关于正确性，你需要用开启空间回收的程序通过 OJ 测试。

### 并发 *(1%)*

对于一个强大的数据库系统来说，并发功能是非常重要的。在本任务中，你需要实现 B+ 树的并发功能。

从 Wikipedia 上可以找到对于并发的定义：

> In computer science, concurrency is the ability of different parts or units of a program, algorithm, or problem to be executed out-of-order or in partial order, without affecting the final outcome.

此 bonus 仅是对于 B+ 树的要求，如果感兴趣，你也可以让你的主体支持高并发操作。此 bonus 无需通过 OJ 测试，但会在 code review 时检查正确性。

**注意:** 准备尝试此任务的同学请务必与助教联系并讨论自己的实现思路。

### 快照 *(0.8%)*

对于一个强大的数据库系统来说，支持快照与恢复功能是非常重要的。在本任务中，你需要实现一个快照系统，实现任意快照间的切换。

一个快照几乎不会占用额外存储空间：

- 在一个状态上创建一个快照，应该只会占用很小的额外存储来记录这个快照本身的信息，不会将快照的内容完全复制一遍；
- 在同样的内容上创建 100 个快照，应该只会占用很小的额外存储，不会使磁盘占用增加 100 倍；
- 在创建快照之后改动一小部分内容，再创建一个快照，也应该只会占用很小的额外存储，不会使磁盘占用增加一倍。

要做到这点，可以利用 [Copy-on-write](https://en.wikipedia.org/wiki/Copy-on-write) 的做法。简单来说，就是令所有块都是不可变的，在试图修改一个块的时候，对这个块做复制，然后在复制出的新块上做修改。但是如果在不快照的时候也对这些块做 copy-on-write 的话，每修改一次数据，就会增加一个新块，这是不可接受的。因此，你需要只在快照的时候做 copy-on-write。

你需要实现以下接口：（以下内容中 `SnapshotID` 为快照 ID，是一个最大长度为 16 的字符串，字符只包含大小写英文字母和数字）
- **创建快照** (create)
  - 在当前状态上创建一个快照，名称为 `SnapshotID`。
    - 如果 `SnapshotID` 已经存在，则不做任何事并抛出异常。
- **还原快照** (restore)
  - 还原到名称为 `SnapshotID` 的快照状态。
    - 如果 `SnapshotID` 不存在，则不做任何事并抛出异常。
- **删除快照** (delete)
  - 删除名称为 `SnapshotID` 的快照状态，不改变当前状态。
    - 其占用的存储空间也需一并释放（下文中说明的情况除外）；
    - 如果 `SnapshotID` 不存在，则不做任何事并抛出异常。

**注意:** 准备尝试此任务的同学请务必与助教联系并讨论自己的实现思路。

### 容错 *(1%)*

程序执行时，我们可能会遇到突然断电等意外情况，导致操作中断。如果此时恰好在对磁盘进行读写操作，那么原本的文件系统就会被破坏，从而造成数据损坏的情况。

在 Unix 中常用的文件系统中，删除一个文件可以分为 3 步：

1. 删除访问的 entry 指针
2. 释放对应的 index node
3. 归还对应的磁盘空间

如果操作发生中断，一个常见的恢复方式是从第一条输入指令开始，重新执行一整轮操作，但是时间成本显然难以接受。

因此，我们可以实现一个 journaling file system，利用日志来恢复文件，保证文件的数据不被损坏。

我们可以考虑把一个指令分为多个 **原子操作** (atomic operation)，在 journal 中保存每个原子操作。当程序中断时，我们就可以知道保存了哪些原子操作，进而进行文件恢复。

**Hint**：请思考一下在不同步骤发生中断时，应该如何选取合适的原子操作，并通过原子操作来恢复文件。 [reference link](https://en.wikipedia.org/wiki/Journaling_file_system)

**注意:** 准备尝试此任务的同学请务必与助教联系并讨论自己的实现思路。