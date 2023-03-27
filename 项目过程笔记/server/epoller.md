# server/epoller.h、server/epoller.cpp

## 使用库

* sys/epoll.h 头文件，epoll - I/O 事件通知工具

```C++
#include <sys/epoll.h>

// epoll_event结构体
struct epoll_event {
    uint32_t     events;    /* Epoll events */
    epoll_data_t data;      /* User data variable */
};

typedef union epoll_data {
    void    *ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

// epoll_ctl 系统调用函数，这个系统调用用于添加、修改或删除文件描述符 epfd 所引用的 epoll 实例的兴趣列表中的条目。 它请求对目标文件描述符 fd 执行op操作。
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

// epoll_wait 系统调用函数，等待文件描述符 epfd 引用的epoll实例上的事件。事件指向的缓冲区用于从就绪列表中返回有关兴趣列表中具有某些可用事件的文件描述符的信息。 最多最大事件由epoll_wait（）返回。maxevents 参数必须大于零。
int epoll_wait(int epfd, struct epoll_event *events,
                int maxevents, int timeout);
```

## 笔记