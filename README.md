# CS144 斯坦福计算机网络课程实验

CS144是斯坦福的计算机网络课程，全程为：CS144: Introduction to Computer，本文是我的 CS144 编程实验的学习总结。

所有LAB完成后的代码放在对应的 labx-solution 分支下，最开始的LAB0的原始代码放在 lab0-start 分支下，请根据自己的需求自行切换。

若因为实验环境问题或其他问题可参考我的博客：https://deepcoding.xyz/archives/cs144--shou-xie-tcp-xie-yi

## 实验

这门课最精彩的部门是实验部分，一步一步指导你实现一个TCP协议，以下是每个实验的概要：

- Lab0：简单的互联网应用 & 读取/写入有序字节流
- Lab1：实现一个流重组器
- Lab2：实现TCP接收方
- Lab3：实现TCP发送方
- Lab4：实现TCP的连接

## lab0 读取/写入 有序字节流（2-3小时）

相关文件：byte_stream.hh  byte_stream.hh

要求实现一个有序字节流类（in-order byte stream），使之支持读写、容量控制。这个字节流类似于一个带容量的队列，从一头读，从另一头写。当流中的数据达到容量上限时，便无法再写入新的数据。特别的，写操作被分为了peek和pop两步。peek为从头部开始读取指定数量的字节，pop为弹出指定数量的字节。

第一反应是搞个循环队列，容器基于数组，长度等于容量，这样内存被充分利用，效率也不错。不过讲义要求我们用“Modern C++”，避免用普通指针，所以我退而求其次用std::deque代替。为什么不用std::queue？因为queue只能访问开头的节点，无法实现peek操作。

另外，除了课程的讲义意外，一定要看sponge的documentary！！！

## lab1 流重组器（5-6小时）

相关文件：stream_reassembler.hh stream_reassmbler.cc

感觉类似于 os 里的进程内存管理，构造一个二叉排序树，树的每一个节点存储碎片的索引 (index)，长度 (length)，数据 (data)。由于碎片可能交叉重叠，每次插入新碎片时，判断能不能和前后的碎片进行合并 (merge)。流的内容可以用一个数组来做缓冲区。因为` std::set` 底层就是用红黑树实现的，所以我就直接拿来用了。

## lab2 TCP接收方（5-6小时）

相关文件：wrapping_integers.cc、wrapping_integers.hh、tcp_receiver.cc、tcp_receiver.hh

要求实现一个基于滑动窗口的 TCP 接收端，其实就是选择重传协议，不过多了一些小细节。

整个接收端的空间由窗口空间（基于StreamReassmbler）和缓存区空间（基于ByteStream）两部分共享。需要注意的是，窗口长度 应该等于接收端容量减去还留在缓冲区的字节数，只有当字节从缓冲区读出后，窗口长度才能缩减。

这节的实验是基于 lab0 和 lab1的，其实大部分我们已经完成了，在 lab2 中我们只需要写一点业务逻辑就行了。就是需要疯狂调试。

## lab3 TCP发送方（6-7小时）

相关文件：tcp_sender.cc、tcp sender.hh

图里用的是“分别确认”的协议，即发送方收到一个ackno只代表接收方收到了该ackno对应的那个段。而我们在这个lab用的是基于“累计确认”的ARQ协议，即发送方收到一个ackno代表了接收方已经收到ackno之前的所有段。原先的时候我花了大部分的时间思考怎么设计分别确认的数据结构，又怎样来跟踪重传的段。换成累计确认就简单多了，直接一个queue就行，重传的时候只传头就行。
![img](https://github.com/Pumpkin9841/cs44-lab/blob/lab0-start/pictures/1.jpg?raw=true)
编码前三思而后行，我建议至少阅读讲义五遍。

## lab4 TCPConnection （7-10小时）

构建TCP的有限状态机（FSM）。虽然讲义里说这节课不需要设计啥新东西，只要拼拼凑凑就行，但实际实现难度比前四个实验加起来还难。主要难点在于TCP的FSM涉及到12种状态间的转换，需要很多的细节逻辑来控制。并且老师说因为不想让大家“面向样例编程”，所以LAB4之前的几个LAB的测试样例并非“全面”的，到了LAB4再给你套“完备”的测试，这就导致很多前几个LAB潜在的BUG都集中在LAB4里爆发出来，需要修改前面实现过的代码才行。

思路分析也实在没啥好说的了，去网上狂啃资料把TCP的FSM吃透，把讲义完全吃透，把官网FAQ里的东西吃透，把libsponge/tcp_helpers/tcp_state.cc吃透。然后疯狂的改BUG吧。

需要把下面三幅图吃透

![](https://github.com/Pumpkin9841/cs44-lab/blob/lab0-start/pictures/2.jpg?raw=true)
![](https://github.com/Pumpkin9841/cs44-lab/blob/lab0-start/pictures/3.png?raw=true)
![](https://github.com/Pumpkin9841/cs44-lab/blob/lab0-start/pictures/4.png?raw=true)


