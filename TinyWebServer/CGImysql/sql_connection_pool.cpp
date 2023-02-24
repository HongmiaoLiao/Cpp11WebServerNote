#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

ConnectionPool::ConnectionPool(): curConn(0),freeConn(0) {
    // 源代码把初始化写在这里用=赋值，但我觉得写列表形式更好
}

ConnectionPool* ConnectionPool::getInstance() {
    static ConnectionPool connPool;
    return &connPool;
} 

//构造初始化
void ConnectionPool::init(string url, string user, string password, string dbName, int port, int maxConn, int closeLog) {
    this->url = url;
    this->port = port;
    this->user = user;
    this->password = password;
    this->dataBaseName = dbName;
    this->closeLog = closeLog;

    for (int i = 0; i < maxConn; ++i) {
        MYSQL* conn = NULL;
        conn = mysql_init(conn);
        if (conn == NULL) {
            LOG_ERROR("MySQL Error"); 
            exit(1);
        }

        conn = mysql_real_connect(conn, url.c_str(), user.c_str(), password.c_str(), dbName.c_str(), port, NULL, 0);

        if (conn == NULL) {
            LOG_ERROR("MySQL Error"); //日志系统内容，还没写
            exit(1);
        }
        connList.push_back(conn);
        ++this->freeConn;
    }

    reserve = Sem(this->freeConn);
    this->maxConn = this->freeConn;
}

// 当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL* ConnectionPool::getConnection() {
    MYSQL* conn = NULL;
    if (connList.size() == 0) {
        return NULL;
    }

    reserve.wait();
    lock.lock();
    conn = connList.front();
    connList.pop_front();
    --this->freeConn;
    ++this->curConn;
    lock.unlock();
    return conn;
}

// 释放当前使用的连接
bool ConnectionPool::releaseConnection(MYSQL* conn) {
    if (conn == NULL)
        return false;
    lock.lock();

    connList.push_back(conn);
    ++this->freeConn;
    --this->curConn;

    lock.unlock();
    reserve.post();
    return true;
}

// 获取当前空闲的连接数
int ConnectionPool::getFreeConn() {
    return this->freeConn;
}

// 销毁数据库连接池
void ConnectionPool::destroyPool() {
    lock.lock();
    if (connList.size() > 0) {
        list<MYSQL*>::iterator it;
        for (it = connList.begin(); it != connList.end(); ++it) {
            MYSQL* conn = *it;
            mysql_close(conn);
        }
        this->curConn = 0;
        this->freeConn = 0;
        connList.clear();
    }
    lock.unlock();
}

ConnectionPool::~ConnectionPool() {
    destroyPool();
}

ConnectionRAII::ConnectionRAII(MYSQL** conn, ConnectionPool* connPool) {
    *conn = connPool->getConnection();
    connRAII = *conn;
    poolRAII = connPool;
}

ConnectionRAII::~ConnectionRAII() {
    poolRAII->releaseConnection(connRAII);
}