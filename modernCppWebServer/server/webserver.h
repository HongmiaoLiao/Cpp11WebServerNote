//
// Created by lhm on 2023/3/22.
//

#ifndef MODERNCPPWEBSERVER_SERVER_WEBSERVER_H_
#define MODERNCPPWEBSERVER_SERVER_WEBSERVER_H_

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer {
 public:
  WebServer(int port, int trig_mode, int timeout_ms, bool opt_linger,
            int sql_port, const char *sql_user, const char *sql_pwd,
            const char *db_name, int conn_pool_num, int thread_num,
            bool open_log, int log_level, int log_que_size);

  ~WebServer();
  void Start();

 private:

  bool InitSocket_();
  void InitEventMode_(int trig_mode);
  void AddClient_(int fd, sockaddr_in addr);

  void DealListen_();
  void DealWrite_(HttpConn *client);
  void DealRead_(HttpConn *client);

  void SendError_(int fd, const char *info);
  void ExtentTime_(HttpConn *client);
  void CloseConn_(HttpConn *client);

  void OnRead_(HttpConn *client);
  void OnWrite_(HttpConn *client);
  void OnProcess(HttpConn *client);

  static const int kMaxFd = 65536;

  static int SetFdNonblock(int fd);

  int port_;
  bool open_linger_;
  int timeout_ms_;
  bool is_close_;
  int listen_fd_;
  char *src_dir_;

  uint32_t listen_event_;
  uint32_t conn_event_;

  std::unique_ptr<HeapTimer> timer_;
  std::unique_ptr<ThreadPool> thread_pool_;
  std::unique_ptr<Epoller> epoller_;
  std::unordered_map<int, HttpConn> users_;
};

#endif //MODERNCPPWEBSERVER_SERVER_WEBSERVER_H_
