#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
// 找不到mysql/mysql.h头文件的时候，需要安装一个库文件：sudo apt install libmysqlclient-dev
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;

class ConnectionPool {
public:
    MYSQL* getConnection(); // 获取数据库连接
    bool releaseConnection(MYSQL* conn);    // 释放连接
    int getFreeConn();  // 获取连接
    void destroyPool(); // 销毁所有连接

    // 单例模式
    static ConnectionPool *getInstance();

    void init(string url, string user, string password, string dataBaseName, int port, int maxConn, int closeLog);

    string url;   // 主机端口号
    int port;  // 数据库端口号 !!注意，源码是string类型，为了和init一样我该为int
    // string port;
    string user;  // 登陆数据库用户名
    string password;  // 登陆数据库密码
    string dataBaseName; // 使用数据库名
    int closeLog;  // 日志开关

private:
    // 单例模式下，将构造函数和析构函数写在private
    ConnectionPool(); // 构造函数写在private，其他类不能直接调用该类生成新的对象，也就避免了同一个类被反复创建的情况，这种情况下，该类只有一个对象实例
    ~ConnectionPool(); // 阻止了用户在类域外对析构函数的使用，用户不能随便删除对象。如果用户想删除对象的话，只能按照类的实现者提供的方法进行。

    int maxConn;  // 最大连接数
    int curConn;  // 当前已使用的连接数
    int freeConn; // 当前空闲的连接数
    Locker lock;
    list<MYSQL*> connList;  //连接池
    Sem reserve;

};

class ConnectionRAII {
public:
    // 这里需要注意的是，在获取连接时，通过有参构造对传入的参数进行修改。其中数据库连接本身是指针类型，所以参数需要通过指针的指针才能对其进行修改。
    ConnectionRAII(MYSQL** conn, ConnectionPool* connPool);
    ~ConnectionRAII();

private:
    MYSQL* connRAII;
    ConnectionPool *poolRAII;
};

#endif