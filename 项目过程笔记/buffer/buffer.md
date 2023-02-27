# buffer/buffer.h, buffer/buffer.c

## 使用库

* cstring 头文件

```C++
#include <cstring>

// bzero 函数从s指向的位置开始擦除内存中n个字节的数据，写入0(包括'\0')，属于Linux下的库函数，在strings.h
void bzero(void *s, size_t n);

```

* iostream头文件

```C++
#include <iostream>

```

* unistd.h头文件

```C++
#include <unistd.h>

```

* sys/uio.h头文件

```C++
#include <sys/uio.h>

```

* vector头文件

```C++
#include <vector>

```

* atomic头文件，定义用于创建支持原子操作的类型的类和类模板。

```C++
#include <atomic>

// atomic 结构，描述对 Ty 类型的存储值执行原子操作的对象。
// 这个对象重载了算术运算符，可以类似基本类型进行加减乘除和位运算
template <class Ty>
struct atomic;

```

* 其他

```C++

```

## 笔记记录

私有成员变量：

* std::vector\<char> buffer_; 存放缓冲区的数据
* std::atomic\<std::size_t> read_pos_; 已经读取缓冲区的字节数，即下一个读取的位置下标
* std::atomic\<std::size_t> write_pos_; 已经写入缓冲区的字节数，即接下来写入的位置下标

私有成员函数：

BeginPtr_: 返回缓冲区的起始地址字符指针，两个重载版本，实现上对第一个元素迭代器解引用得到第一个元素，再取地址就是第一个字符的地址

MakeSpace_：为缓冲区腾出len大小的空间

* 如果可写的空间加上已读的可以重新写入的空间不足len，则为缓冲区容器重新分配空间，大小相当于是已写的空间再加上len长度的空间
* 否则，将未读的内容移动到缓冲区最前方，其他内容就相当于清空了，腾出后面的空间

有参构造函数：

析构函数：

公有成员函数：

WritableBytes：返回剩下可写入的空间大小，为缓冲区总大小减去已写入大小

ReadableBytes：返回可读取的内容长度，已写入长度减已读长度为未读长度

PrependableBytes：返回已读的可以重新写入的大小，即返回已读的位置

Peek：返回缓冲区已读内容的顶端地址，为起始指针向后移动已读长度

EnsureWritable

HasWritten

Retrieve：检索len个字节，即已读长度向后移动增加len（判断可读长度大于len）

RetrieveUntil：向后检索直到传入的*end指针位置，通过指针计算出需要向后读的长度，再调用Retrieve（判断传入指针在已读顶端之后

RetrieveAll：清空所有缓存，将已读已写长度置为0，将缓存空间内容全清0

RetrieveAllToStr：检索出所有未读的数据，以string类型返回，并清空所有缓存

BeginWriteConst：返回写入位置的指针，即起始指针向后移动已写长度，const版

BeginWrite：返回写入位置的指针，即起始指针向后移动已写长度

Append

ReadFd

WriteFd
