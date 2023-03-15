//
// Created by lhm on 2023/3/6.
//

#include "sqlconnpool.h"

SqlConnPool::SqlConnPool() : use_count_(0), free_count_(0) {
}

// 创建数据库连接池静态变量，返回其地址（指针）
SqlConnPool *SqlConnPool::Instance() {
  static SqlConnPool conn_pool;
  return &conn_pool;
}

// 初始化数据库连接池，创建连接线程，初始化信号量
void SqlConnPool::Init(const char *host,
                       int port,
                       const char *user,
                       const char *pwd,
                       const char *db_name,
                       int conn_size) {
  // 连接数量要大于0
  assert(conn_size > 0);
  // 用一个循环创建conn_size个连接
  for (int i = 0; i < conn_size; ++i) {
    MYSQL *sql = nullptr;
    // 初始化MYSQL结构，如果失败，记录日志
    sql = mysql_init(sql);
    if (!sql) {
      LOG_ERROR("MySql init error!");
      assert(sql);
    }
    // 建立数据库连接，如果失败，记录日志
    sql = mysql_real_connect(sql, host, user, pwd, db_name, port, nullptr, 0);
    if (!sql) {
      LOG_ERROR("MySql Connect error!");
    }
    // 将一个连接放入连接池队列（初始化不用加锁）
    conn_que_.push(sql);
  }
  // 记录连接池池大小
  max_count_ = conn_size;
  // 初始化信号量为max_count_
  sem_init(&sem_id_, 0, max_count_);
}

// 从数据库连接池中获取一个连接
MYSQL *SqlConnPool::GetConn() {
  MYSQL *sql = nullptr;
  if (conn_que_.empty()) {
    // 如果连接池为空，即没有可用连接，记录日志并返回空指针
    LOG_WARN("SqlConnPoll busy!");
    return nullptr;
  }
  // 减少一个信号量
  sem_wait(&sem_id_);
  {
    // 上锁后从数据库连接池队列取出一个连接
    std::lock_guard<std::mutex> locker(mtx_);
    sql = conn_que_.front();
    conn_que_.pop();
  }
  return sql;
}

// 释放一个连接conn，将其返回数据库连接池队列
void SqlConnPool::FreeConn(MYSQL *conn) {
  assert(conn);
  std::lock_guard<std::mutex> locker(mtx_);
  conn_que_.push(conn);
  sem_post(&sem_id_);
}

// 关闭数据库连接池
void SqlConnPool::ClosePool() {
  std::lock_guard<std::mutex> locker(mtx_); // 访问队列时上锁
  while (!conn_que_.empty()) {
    // 弹出连接池队列所有连接，将其关闭
    auto item = conn_que_.front();
    conn_que_.pop();
    mysql_close(item);
  }
}

// 返回当前可用连接数量，即连接池队列的当前大小
int SqlConnPool::GetFreeConnCount() {
  std::lock_guard<std::mutex> locker(mtx_);
  return conn_que_.size();
}

SqlConnPool::~SqlConnPool() {
  ClosePool();
}






