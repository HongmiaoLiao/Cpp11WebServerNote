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
  static SqlConnPool *Instance();

  MYSQL *GetConn();
  void FreeConn(MYSQL *conn);
  int GetFreeConnCount();

  void Init(const char *host, int port,
            const char *user, const char *pwd,
            const char *db_name, int conn_size);
  void ClosePool();

 private:
  SqlConnPool();
  ~SqlConnPool();

  int max_count_;
  int use_count_;
  int free_count_;

  std::queue<MYSQL *> conn_que_;
  std::mutex mtx_;
  sem_t sem_id_;
};

#endif //MODERNCPPWEBSERVER_POOL_SQLCONNPOOL_H_
