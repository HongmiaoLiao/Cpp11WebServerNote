//
// Created by lhm on 2023/3/6.
//

#ifndef MODERNCPPWEBSERVER_POOL_SQLCONNRAII_H_
#define MODERNCPPWEBSERVER_POOL_SQLCONNRAII_H_

#include "sqlconnpool.h"

/* 原作者注释：资源在对象构造初始化，资源在对象析构时释放 */
class SqlConnRAII {
 public:
  SqlConnRAII(MYSQL **sql, SqlConnPool *conn_pool) {
    assert(conn_pool);
    *sql = conn_pool->GetConn();
    sql_ = *sql;
    conn_pool_ = conn_pool;
  }

  ~SqlConnRAII() {
    if (sql_) {
      conn_pool_->FreeConn(sql_);
    }
  }

 private:
  MYSQL *sql_;  // 一个与数据库的连接的指针
  SqlConnPool *conn_pool_;  // 数据库连接池的指针
};

#endif //MODERNCPPWEBSERVER_POOL_SQLCONNRAII_H_
