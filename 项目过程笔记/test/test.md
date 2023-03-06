# test.h

## 使用库

* features.h 头文件,定义是否包含算法变量。较少的变量可以减少可执行文件的大小和编译时间。该文件是标准c++库的GNU并行扩展。

```C++
#include <features.h>

__GLIBC__、__GLIBC_MINOR__
是版本信息的宏？
```

* sys/syscall.h 头文件，间接系统调用

```C++
#include <sys/syscall.h>

// syscall 函数 执行一个系统调用，根据指定的参数number和所有系统调用的汇编语言接口来确定调用哪个系统调用。
int syscall(int number, ...);

syscall(SYS_gettid)可获得当前线程的id

// 各个进程独立，所以会有不同进程中线程号相同节的情况。那么这样就会存在一个问题，我的进程p1中的线程pt1要与进程p2中的线程pt2通信怎么办，进程id不可以，线程id又可能重复，所以这里会有一个真实的线程id唯一标识，tid。glibc没有实现gettid的函数，所以我们可以通过linux下的系统调用syscall(SYS_gettid)来获得。
```

* functional 头文件

```C++
#include <functional>

// bind 对象，将自变量绑定到可调用对象。
template <class FT, class T1, class T2, ..., class TN>
    unspecified bind(FT fn, T1 t1, T2 t2, ..., TN tN);
```

## 笔记

这里定义了对日志、线程池的测试，当然，也间接测试了缓冲区，阻塞队列

TestLog函数：非异步写测试

ThreadLogTask函数，10000次日志写入，写入自己的线程和一个计数值

TestThreadPool函数：利用线程池，产生多个日志写入动作线程，然后异步写入
