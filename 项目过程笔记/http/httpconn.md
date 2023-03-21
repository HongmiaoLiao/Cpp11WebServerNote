# http/httpconn.h、http/httpconn.cpp

## 使用库

* arpa/inet.h 头文件, Internet操作的定义

```C++
#include <arpa/inet.h>

// sockaddr_in 结构体，用来处理网络通信的地址。
struct sockaddr_in {
    sa_family_t    sin_family; /* address family: AF_INET */
    in_port_t      sin_port;   /* port in network byte order */
    struct in_addr sin_addr;   /* internet address */
};
```

* 其他

c++ 静态变量，经常会放到cpp文件中初始化。但并非一定要放到cpp中初始化.

之所以需要放到cpp中初始化，是因为static变量，必需切只能一次被初始化。

如果放到头文件.h中，两个cpp都include了.h文件，那就变成了"multiple definition"。但是如果只会被include一次，放到.h中是可以编译通过的。

问题的核心在于 *必需切只能一次被初始化*

## 笔记
