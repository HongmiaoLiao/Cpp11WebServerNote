//
// Created by lhm on 2023/3/22.
//

#include "webserver.h"
WebServer::WebServer(int port, int trig_mode, int timeout_ms, bool opt_linger,
                     int sql_port, const char *sql_user, const char *sql_pwd,
                     const char *db_name, int conn_pool_num, int thread_num,
                     bool open_log, int log_level, int log_que_size)
    : port_(port), open_linger_(opt_linger), timeout_ms_(timeout_ms), is_close_(false),
      timer_(new HeapTimer()), thread_pool_(new ThreadPool(thread_num)), epoller_(new Epoller) {
  src_dir_ = getcwd(nullptr, 256);
  assert(src_dir_);
  strncat(src_dir_, "/resources", 16);
  HttpConn::user_count = 0;
  HttpConn::src_dir = src_dir_;
  SqlConnPool::Instance()->Init("localhost", sql_port, sql_user,
                                sql_pwd, db_name, conn_pool_num);

  InitEventMode_(trig_mode);
  if (!InitSocket_()) {
    is_close_ = true;
  }

  if (open_log) {
    Log::Instance()->Init(log_level, "./log", ".log", log_que_size);
    if (is_close_) {
      LOG_ERROR("======== Server init error! ========");
    } else {
      LOG_INFO("======== Server init ========");
      LOG_INFO("Port:%d, OpenLinger: %s", port_, opt_linger ? "ture" : "false");
      LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
               (listen_event_ & EPOLLET ? "ET" : "LT"),
               (conn_event_ & EPOLLET ? "ET" : "LT"));
      LOG_INFO("LogSys level: %d", log_level);
      LOG_INFO("srcDir: %s", HttpConn::src_dir);
      LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", conn_pool_num, thread_num);
    }
  }
}

WebServer::~WebServer() {
  close(listen_fd_);
  is_close_ = true;
  free(src_dir_);
  SqlConnPool::Instance()->ClosePool();
}

void WebServer::InitEventMode_(int trig_mode) {
  listen_event_ = EPOLLRDHUP;
  conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
  switch (trig_mode) {
    case 0:break;
    case 1:conn_event_ |= EPOLLET;
      break;
    case 2:listen_event_ |= EPOLLET;
      break;
    case 3:listen_event_ |= EPOLLET;
      conn_event_ |= EPOLLET;
      break;
    default:listen_event_ |= EPOLLET;
      conn_event_ |= EPOLLET;
      break;
  }
  HttpConn::isET = (conn_event_ & EPOLLET);
}

void WebServer::Start() {
  int time_ms = -1; /* 原作者注释 epoll wait timeout == -1 无事件将阻塞 */
  if (!is_close_) {
    LOG_INFO("======== Server start ========");
  }
  while (!is_close_) {
    if (timeout_ms_ > 0) {
      time_ms = timer_->GetNextTick();
    }
    int event_cnt = epoller_->Wait(time_ms);
    for (int i = 0; i < event_cnt; ++i) {
      /* 原作者注释 处理事件 */
      int fd = epoller_->GetEventFd(i);
      uint32_t events = epoller_->GetEvents(i);
      if (fd == listen_fd_) {
        DealListen_();
      } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        assert(users_.count(fd) > 0);
        CloseConn_(&users_[fd]);
      } else if (events & EPOLLIN) {
        assert(users_.count(fd) > 0);
        DealRead_(&users_[fd]);
      } else if (events & EPOLLOUT) {
        assert(users_.count(fd) > 0);
        DealWrite_(&users_[fd]);
      } else {
        LOG_ERROR("Unexpected event");
      }
    }
  }
}

void WebServer::SendError_(int fd, const char *info) {
  assert(fd > 0);
  int ret = send(fd, info, strlen(info), 0);
  if (ret < 0) {
    LOG_WARN("send error to client[%d] error!", fd);
  }
  close(fd);
}

void WebServer::CloseConn_(HttpConn *client) {
  assert(client);
  LOG_INFO("Client[%d] quit!", client->GetFd());
  epoller_->DelFd(client->GetFd());
  client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
  assert(fd > 0);
  users_[fd].Init(fd, addr);
  if (timeout_ms_ > 0) {
    timer_->Add(fd, timeout_ms_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
  }
  epoller_->AddFd(fd, EPOLLIN | conn_event_);
  SetFdNonblock(fd);
  LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::DealListen_() {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);

  do {
    int fd = accept(listen_fd_, (struct sockaddr *) &addr, &len);
    if (fd <= 0) {
      return;
    } else if (HttpConn::user_count >= kMaxFd) {
      SendError_(fd, "Server busy!");
      LOG_WARN("Clients is full!");
      return;
    }
    AddClient_(fd, addr);
  } while (listen_event_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn *client) {
  assert(client);
  ExtentTime_(client);
  thread_pool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn *client) {
  assert(client);
  ExtentTime_(client);
  thread_pool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(HttpConn *client) {
  assert(client);
  if (timeout_ms_ > 0) {
    timer_->Adjust(client->GetFd(), timeout_ms_);
  }
}

void WebServer::OnRead_(HttpConn *client) {
  assert(client);
  int ret = -1;
  int read_errno = 0;
  ret = client->Read(&read_errno);
  if (ret <= 0 && read_errno != EAGAIN) {
    CloseConn_(client);
    return;
  }
  OnProcess(client);
}

void WebServer::OnProcess(HttpConn *client) {
  if (client->Process()) {
    epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
  } else {
    epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLIN);
  }
}

void WebServer::OnWrite_(HttpConn *client) {
  assert(client);
  int ret = -1;
  int write_errno = 0;
  ret = client->Write(&write_errno);
  if (client->ToWriteBytes() == 0) {
    /* 原作者注释 传输完成 */
    if (client->IsKeepAlive()) {
      OnProcess(client);
      return;
    }
  } else if (ret < 0) {
    if (write_errno == EAGAIN) {
      /* 原作者注释 继续传输 */
      epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
      return;
    }
  }
  CloseConn_(client);
}

/* 原作者注释 Create listenfd */
bool WebServer::InitSocket_() {
  int ret;
  struct sockaddr_in addr;
  if (port_ > 65535 || port_ < 1024) {
    LOG_ERROR("Port:%d error!", port_);
    return false;
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);
  struct linger opt_linger = {0};
  if (open_linger_) {
    /* 原作者注释 优雅关闭，直到所剩数据发送完毕或超时 */
    opt_linger.l_onoff = 1;
    opt_linger.l_linger = 1;
  }

  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    LOG_ERROR("Create socket error!", port_);
    return false;
  }

  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
  if (ret < 0) {
    close(listen_fd_);
    LOG_ERROR("Init linger error!", port_);
    return false;
  }

  int optval = 1;
  /* 原作者注释 端口复用 */
  /* 只有最后一个套接字会正常接收数据 */
  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));
  if (ret == -1) {
    LOG_ERROR("set socket setsockopt error!");
    close(listen_fd_);
    return false;
  }

  ret = bind(listen_fd_, (struct sockaddr *) &addr, sizeof(addr));
  if (ret < 0) {
    LOG_ERROR("Bind Port:%d error!", port_);
    close(listen_fd_);
    return false;
  }

  ret = listen(listen_fd_, 6);
  if (ret < 0) {
    LOG_ERROR("Listen port:%d error!", port_);
    close(listen_fd_);
    return false;
  }
  ret = epoller_->AddFd(listen_fd_, listen_event_ | EPOLLIN);
  if (ret == 0) {
    LOG_ERROR("Add listen error!");
    close(listen_fd_);
    return false;
  }
  SetFdNonblock(listen_fd_);
  LOG_INFO("Server port:%d", port_);
  return true;
}

int WebServer::SetFdNonblock(int fd) {
  assert(fd > 0);
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}






