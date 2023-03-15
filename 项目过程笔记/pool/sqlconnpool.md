# pool/sqlconnpool.h、pool/sqlconnpool.cpp

## 使用库

* mysql/mysql.h 头文件

```C++
#include <mysql/mysql.h>

// MYSQL 结构，包含用户名、密码、数据库、线程id等等众多数据库状态相关信息
struct MYSQL {...};

// mysql_init函数，初始化一个MYSQL结构，返回其指针
MYSQL *STDCALL mysql_init(MYSQL *mysql)
// STDCALL是个空的宏，应该是作为标志防止重复定义吧
#define STDCALL

// mysql_real_connect 函数，获取数据库连接，返回空表示连接失败，否则返回数据库连接信息的句柄（handle）
MYSQL *STDCALL mysql_real_connect(MYSQL *mysql, 
    const char *host, const char *user, const char *passwd, 
    const char *db, unsigned int port, const char *unix_socket, 
    unsigned long clientflag)


```

* semaphore.h头文件

```C++
#include <semaphore.h>

// sem_t 类型，本质上是一个长整型的数, 信号量的数据类型
sem_t

// sem_init 函数，在地址sem处初始化信号量。value参数指定了初始值
int sem_init(sem_t *sem, int pshared, unsigned int value);
// pshared参数指示这个信号量是否存在在进程的线程之间或进程之间共享；如果pshared的值为0，则该信号量将被进程的线程共享

// sem_wait 函数，减少（上锁）sem指向的信号量，如果信号量大于0，则减1后返回，否则，阻塞直到有信号量可以减，成功返回0，失败返回-1
int sem_wait(sem_t *sem);
```

## 笔记

数据库连接池类，采用单例模式，包含连接池进行初始化，对连接的存取操作

私有成员变量：

* max_count_：数据库连接池的连接数量
* use_count_：初始化后未使用
* free_count_：初始化后未使用
* conn_que_：数据库连接队列
* mtx_：互斥锁，负责连接池队列的互斥访问
* sem_id_：

私有构造函数：初始化use_count_, free_count_

私有析构函数：调用ClosePool()函数关闭连接池

公有成员函数：

Instance：创建数据库连接池静态变量，返回其地址（指针）

GetConn：从数据库连接池中获取一个连接，为空返回空指针，否则返回此连接

* 如果连接池为空，即没有可用连接，记录日志并返回空指针
* 上减少一个信号量，上锁后从数据库连接池队列取出一个连接

FreeConn：释放一个连接conn，将其返回数据库连接池队列

GetFreeConnCount：返回当前可用连接数量，即连接池队列的当前大小

Init：初始化数据库连接池，创建连接线程，初始化信号量

* 连接数量要大于0
* 用一个循环创建conn_size个连接
  * 初始化MYSQL结构，如果失败，记录日志
  * 建立数据库连接，如果失败，记录日志
  * 将一个连接放入连接池队列
* 初始化信号量为max_count_

ClosePool：关闭数据库连接池，上锁后，弹出连接池队列所有连接，将其关闭
