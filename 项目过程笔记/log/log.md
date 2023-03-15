# log/log.h、log/log.cpp

## 使用库

* cstdio 头文件，包含标准 C 库标头 \<stdio.h>，并将关联的名称添加到 std 命名空间。

```C++
#include <cstdio>

// FILE 类型，标识流并包含控制流所需信息的对象类型，包括指向其缓冲区的指针、位置指示器和所有状态指示器。
FILE

// fflush 函数，将未写入内容写入文件
int fflush ( FILE * stream );
// 如果给定的流是打开写的(或者如果它是打开更新的，并且最后的i/o操作是输出操作)，它的输出缓冲区中任何未写的数据都被写入文件。如果stream是空指针，所有这样的流都将被刷新。

// fputs 函数，将由str指向的C字符串写入流。
int fputs ( const char * str, FILE * stream );
// 函数从指定的地址(str)开始复制，直到到达结束空字符('\0')。此终止空字符不会复制到流中。

// snprintf 函数，格式化输出写入大小为n的缓冲区s
int snprintf ( char * s, size_t n, const char * format, ... );

// fclose 函数，关闭与流关联的文件并将其解除关联。
int fclose ( FILE * stream );

// fopen 函数，打开文件名在参数filename中指定的文件，并将其与一个流关联，该流可在未来操作中由返回的FILE指针识别。
FILE * fopen ( const char * filename, const char * mode );

// vsnprintf 函数，从变量参数列表写入格式化数据到缓冲区s
int vsnprintf (char * s, size_t n, const char * format, va_list arg );

// fputs 函数，将str指向的C风格字符串（到 '\0'结束）写入文件流中
int fputs ( const char * str, FILE * stream );
```

* memory 头文件

```C++
#include <memory>

//存储指向拥有的对象或数组的指针。 此对象/数组仅由 unique_ptr 拥有。 unique_ptr 被销毁后，此对象/数组也将被销毁。
template <class T, class D = default_delete<T>> class unique_ptr;
//只允许基础指针的一个所有者。 除非你确信需要 shared_ptr，否则请将该指针用作 POCO 的默认选项。 可以移到新所有者，但不会复制或共享。 替换已弃用的 auto_ptr。 与 boost::scoped_ptr 比较。 unique_ptr 小巧高效；大小等同于一个指针且支持 rvalue 引用，从而可实现快速插入和对 C++ 标准库集合的检索。
```

* thread 头文件

```C++

// thread类的成员函数joinable()，指定关联的线程是否可联接。如果线程对象表示一个执行线程，那么它就是可联接的。
bool joinable() const noexcept;

// thread类的成员函数join()，阻塞，直到完成与调用对象相关联的执行线程。调用此函数后，线程对象变得不可连接，可以安全地销毁。
bool joinable() const noexcept;

```

* sys/time.h 头文件，该头文件包含获取和操作日期和时间信息的函数定义。包含标准 C 库标头 <time.h> 并将关联名称添加到 std 命名空间。
  * \<sys/time>是POSIX头文件不是C/ c++标准库的一部分。如果使用c++，则标准标头为\<ctime>

```C++
#include <sys/time.h>

// time_t 类型，一种能够表示时间的基本算术类型的别名，它通常被实现为表示从UTC 1970年1月1日00:00小时开始经过的秒数的整数值(即unix时间戳)
time_t

// time 函数，获取当前日历时间为类型为time_t的值，如果参数不是空指针，它还将此值设置为timer所指向的对象。
time_t time (time_t* timer);

// struct tm 结构体，该结构包含日历日期和时间。
struct tm
{
  int tm_sec;  /* Seconds. [0-60] (1 leap second) */
  int tm_min;  /* Minutes. [0-59] */
  int tm_hour;  /* Hours. [0-23] */
  int tm_mday;  /* Day.  [1-31] */
  int tm_mon;  /* Month. [0-11] */
  int tm_year;  /* Year - 1900.  */
  int tm_wday;  /* Day of week. [0-6] */
  int tm_yday;  /* Days in year.[0-365] */
  int tm_isdst;  /* DST. [-1/0/1]*/
};

// localtime 函数，使用timer指向的值，用表示本地时区对应时间的值填充tm结构。
struct tm * localtime (const time_t * timer);

// timeval 结构，里面放秒和毫秒
struct timeval {
  time_t      tv_sec;     /* seconds */
  suseconds_t tv_usec;    /* microseconds */
};

// gettimeofday 函数，tv是要赋值的时间结构，tz是时区信息，为空指针时获取本地时区时间
int gettimeofday(struct timeval *restrict tv,
                struct timezone *restrict tz);
```

* sys/stat.h 头文件

```C++
#include <sys/stat.h>

// mkdir 函数，创建一个名为path的新目录。新目录的文件权限位从mode初始化。mode参数的这些文件权限位由进程的文件创建掩码修改。
int mkdir(const char *path, mode_t mode);
```

* cstdarg 头文件，此头文件定义宏来访问未命名参数列表中的单个参数，这些参数的数量和类型被调用函数不知道。

```C++
#include <cstdarg>

// va_list 类型，保存关于变量参数的信息
va_list

// va_start 宏，初始化一个变量参数列表，初始化ap以检索参数paramN之后的附加参数。
void va_start (va_list ap, paramN); 

// va_end 宏，执行适当的操作，使使用va_list对象ap检索其附加参数的函数能够正常返回。
void va_end (va_list ap);

// 以上三个应该一起使用
```

* 其他

```C++

// 若要使用 variadic 宏，可以在宏定义中将省略号指定为最终形参，并且可以在定义中使用替换标识符 __VA_ARGS__ 来插入额外参数。 __VA_ARGS__ 将由与省略号匹配的所有参数（包括它们之间的逗号）替换。
__VA_ARGS__
```

## 笔记

日志类，采用单例模式，实现对日志的同步或者异步写

一些带锁的方法虽然看不出有成本变量的改变，但其实锁是有在变的，所以不能加const

私有成员变量：

* kLogPathLen：常量，表示最大日志文件路径的长度为256（没有用上）
* kLogNameLen：常量，表示日志文件名的最长名字为256
* kMaxLines：常量，表示日志文件中最大行数为50000
* path_：字符串常量的指针，指向日志文件路径
* suffix_：字符串常量的指针，指向日志文件后缀名
* max_lines_：未使用
* line_count_：表示当前日志的行数
* today_：表示当前为本月第几天
* is_open_：表示日志系统是否开启
* buff_：日志内容缓冲区
* level_：日志等级，一共0，1，2，3四个等级，对应debug,info,warn,error
* is_async_：表示是否异步写入
* fp_：文件类型指针，用于对日志文件的写入
* deque_：独占指针，指向存放日志信息的阻塞队列
* write_thread_：独占指针，指向将日志信息写入文件的线程
* mtx_：互斥锁，用于日志文件的互斥写入

私有构造函数：不能直接构造，只能用静态的公有成员函数Instance获取唯一一个Log对象。只初始化一些成员变量

私有虚析构函数：因为有指针成员fp_，需要自己写虚构函数进行释放，加上virtual可能防止这个类被继承后析构出现问题？

* 如果写线程指针不为空且线程还在执行
  * 关闭阻塞队列
    * 阻塞队列不为空的话，将消费者线程唤醒使其消费完队列内容
  * 阻塞线程，使线程对象可以安全销毁
* 如果文件指针不为空
  * 上锁后，消费完队列内容，写入未写入内容，关闭文件指针

私有成员函数：

AppendLogLevelTitle_：根据日志等级，向缓冲区放入相应的日志头部信息字符串

AsyncWrite_：异步写，将阻塞队列的内容全部写入文件

公有成员函数：

init：执行日志系统的初始化：设置打开标识、日志等级、日志文件路径和后缀名、设置最大日志队列容量，初始化行数为0

* 如果日志队列容量大于，标识为异步，否则不为异步
  * 如果阻塞队列指针为空，即还未创建，则用独占指针指向一个新建的存放string的阻塞队列，然后将其move给私有成员的deque_
  * 新建一个独占指针指向执行异步写任务的线程对象，然后将其move给私有成员的deque
* 获取当前时间，和路径、后缀名拼接作为日志文件名，设置私有变量的今天日期
* 上锁后，清空缓冲区，如果文件指针没有关闭，先将其内容写入后关闭，再重新打开

Instance：创建静态日志对象，获取日志对象的指针，单例模式获取对象的方法

FlushLogThread：调用异步写，将队列内容全部写入文件

Write：根据初始化的信息和当前时间，实现对可变参数内容的写入操作

* 获取当前时间
* 如果日志时间不为今天或日志行数已近满了，则新建日志文件，用当前时间拼接名字，更新日志时间
* 上锁后，写入新的日志文件中
* 在额外开一个作用域上锁，将当前时间信息写入缓冲区，将日志等级信息写入缓冲区
* 读取传入的可变参数，将所有参数以字符串写入缓冲区
* 如果是异步写，则将缓冲区的内容转换为string形式作为阻塞队列的一项，有异步写线程从阻塞队列读取（消费）字符串写入文件；否则直接将缓冲区内容写入文件
* 最后清空缓冲区

Flush：将未写入文件的日志内容写入

GetLevel：获取日志等级，即获取私有成员变量日志标识，设置和获取需要上锁

SetLevel：设置日志等级，即设置私有成员变量日志标识，设置和获取需要上锁

IsOpen：判断日志是否关闭，即然后是否关闭的成员变量标识

最后将日志定义为一个可以使用可变参数的宏，作用是：在日志打开且日志等级正确的情况下，写入相应等级的日志
