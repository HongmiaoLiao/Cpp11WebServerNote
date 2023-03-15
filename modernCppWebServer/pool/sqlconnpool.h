//
// Created by lhm on 2023/3/6.
//

#ifndef MODERNCPPWEBSERVER_POOL_SQLCONNPOOL_H_
#define MODERNCPPWEBSERVER_POOL_SQLCONNPOOL_H_

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
 public:
  // 创建数据库连接池静态变量，返回其地址（指针）
  static SqlConnPool *Instance();

  // 从数据库连接池中获取一个连接，为空返回空指针，否则返回此连接
  MYSQL *GetConn();
  // 释放一个连接conn，将其返回数据库连接池队列
  void FreeConn(MYSQL *conn);
  // 返回当前可用连接数量，即连接池队列的当前大小
  int GetFreeConnCount();

  // 初始化数据库连接池，创建连接线程，初始化信号量
  void Init(const char *host, int port,
            const char *user, const char *pwd,
            const char *db_name, int conn_size);
  // 关闭数据库连接池
  void ClosePool();

 private:
  SqlConnPool();
  ~SqlConnPool();

  int max_count_; // 数据库连接池的连接数量
  int use_count_; // 初始化后未使用
  int free_count_;  // 初始化后未使用

  std::queue<MYSQL *> conn_que_;  // 数据库连接队列
  std::mutex mtx_;
  sem_t sem_id_;
};

#endif //MODERNCPPWEBSERVER_POOL_SQLCONNPOOL_H_
