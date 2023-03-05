# buffer/buffer.h, buffer/buffer.c

## 使用库

* cstring 头文件

```C++
#include <cstring>

// bzero 函数从s指向的位置开始擦除内存中n个字节的数据，写入0(包括'\0')，属于Linux下的库函数，在strings.h
void bzero(void *s, size_t n);

```

* unistd.h头文件

```C++
#include <unistd.h>

// write，系统调用write从指向buf的缓冲区写入文件描述符fd引用的文件，最多写入count字节。
ssize_t write(int fd, const void *buf, size_t count);
```

* sys/uio.h头文件，定义了向量I/O操作

```C++
#include <sys/uio.h>

// iovec 结构体，通常，这个结构用作一个多元素的数组，调用readv或writev向多个缓冲区读或写。
// 第一个为用于输入或输出的内存区域的基址。第二个为iov_base所指向的内存大小。
struct iovec {
    void  *iov_base;    /* Starting address */
    size_t iov_len;     /* Number of bytes to transfer */
};

// readv 函数，readv()系统调用将iovcnt缓冲区从与文件描述符fd相关联的文件读取到由iov(“分散输入”)描述的缓冲区。
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

// read和write的衍生函数，readv和writev可以在一个原子操作中读取或写入多个缓冲区
```

* vector头文件，数组大小可以改变的序列容器，不赘述

* atomic头文件，定义用于创建支持原子操作的类型的类和类模板。

```C++
#include <atomic>

// atomic 结构，描述对 Ty 类型的存储值执行原子操作的对象。
// 这个对象重载了算术运算符，可以类似基本类型进行加减乘除和位运算
template <class Ty>
struct atomic;

```

* algorithm头文件，

```C++
#include <algorithm>
// copy 函数，将范围[first,last)中的元素复制到从result开始的范围。
template<class InputIterator, class OutputIterator>
OutputIterator copy(
    InputIterator first,
    InputIterator last,
    OutputIterator destBeg);
```

* 其他

```C++
// ssize_t类型
// 简而言之，ssize_t与size_t相同，但它是一个有符号类型，将ssize_t读做“有符号的size_t”。ssize_t能够表示数字-1，该数字由多个系统调用和库函数返回，作为指示错误的一种方式。
// ssize_t在POSIX下至少能存放[-1~~2^15 -1]范围的值
ssize_t a = -1;

#include <errno.h>

// 变量 errno，最后一次错误的的错误码，该变量由系统调用和一些库函数在发生错误时设置
errno
```

## 笔记记录

缓冲区类，实现对缓冲区的读写和维护

私有成员变量：

* std::vector\<char> buffer_; 存放缓冲区的数据
* std::atomic\<std::size_t> read_pos_; 已经读取缓冲区的字节数，即下一个读取的位置下标
* std::atomic\<std::size_t> write_pos_; 已经写入缓冲区的字节数，即接下来写入的位置下标

私有成员函数：

BeginPtr_: 返回缓冲区的起始地址字符指针，两个重载版本，实现上对第一个元素迭代器解引用得到第一个元素，再取地址就是第一个字符的地址

MakeSpace_：为缓冲区腾出len大小的空间

* 如果可写的空间加上已读的可以重新写入的空间不足len，则为缓冲区容器重新分配空间，大小相当于是已写的空间再加上len长度的空间
* 否则，将未读的内容移动到缓冲区最前方，其他内容就相当于清空了，腾出后面的空间

有参构造函数：初始化缓冲区大小，默认为1024

析构函数：=default由编译器生成

公有成员函数：

WritableBytes：返回剩下可写入的空间大小，为缓冲区总大小减去已写入大小

ReadableBytes：返回可读取的内容长度，已写入长度减已读长度为未读长度

PrependableBytes：返回已读的可以重新写入的大小，即返回已读的位置

Peek：返回缓冲区已读内容的顶端地址，为起始指针向后移动已读长度

EnsureWritable：确保有len个长度的可写空间，如果没有，则调用MakeSpace成员函数腾出空间

HasWritten：标识向后写len个长度，即已写位置向后移动len个位置

Retrieve：检索len个字节，即已读长度向后移动增加len（判断可读长度大于len）

RetrieveUntil：向后检索直到传入的*end指针位置，通过指针计算出需要向后读的长度，再调用Retrieve（判断传入指针在已读顶端之后

RetrieveAll：清空所有缓存，将已读已写长度置为0，将缓存空间内容全清0

RetrieveAllToStr：检索出所有未读的数据，以string类型返回，并清空所有缓存

BeginWriteConst：返回写入位置的指针，即起始指针向后移动已写长度，const版

BeginWrite：返回写入位置的指针，即起始指针向后移动已写长度

Append：有四个重载，向缓存区写入内容

* 参数为const char* 和size_t的为主要的实现，其他三个调用了这个函数
  * 先确保有len长的可写空间，然后将字符串写入缓冲区，并标识写了len长
* 传入参数为const void*，会调用强制类型转换，转换为const char*
* 传入参数为std::string, 取出string容器中的数据，再调用void Buffer::Append(const void* data, size_t len)。string容器内的数据就是void类型指针指向的字符串
* 传入参数为const Buffer&，则取出buffer内的已读内容顶端和可读内容，再放入对象自己的缓冲符

ReadFd：文件描述符fd中读取内容到缓冲区，并保存可能出现的错误码，返回写入的字节数

* 分散读，保证数据全部读完
* 开辟一个足够大的临时缓冲区（64kb），同时从fd中读取内容到临时缓冲区和对象自己的缓存区
* 如果读取失败，保存错误码
* 如果读取的字节数小于可写的缓冲区空间，直接往后写len字节（其实在readv已经写入，这里将写入位置的标识移动）
* 如果读取的字节数大于可写的缓冲区空间，将写入位置移动到最后，调用Append从临时缓冲区中把没有写入的内容写入到缓存区（Append内部调用MakeSpace_分配足够的空间，再将写位置从当前位置后移）

WriteFd：向文件描述符写入缓冲区中可读的字节，并保存可能出现的错误码，返回写入的字节数

* 如果写入失败，保存错误码，直接返回

值得注意的是，Linux系统将所有设备都当作文件来处理，而Linux用文件描述符来标识每个文件对象。 文件描述符是一个非负整数，用于唯一标识计算机操作系统中打开的文件。 它描述了数据资源，以及如何访问该资源。
