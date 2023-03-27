//
// Created by lhm on 2023/3/22.
//

#ifndef MODERNCPPWEBSERVER_SERVER_EPOLLER_H_
#define MODERNCPPWEBSERVER_SERVER_EPOLLER_H_

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller {
 public:
  explicit Epoller(int max_event = 1024);

  ~Epoller();

  bool AddFd(int fd, uint32_t events);

  bool ModFd(int fd, uint32_t events);

  bool DelFd(int fd);

  int Wait(int timeout_ms = -1);

  int GetEventFd(size_t i) const;

  uint32_t GetEvents(size_t i) const;

 private:
  int epoll_fd_;

  std::vector<struct epoll_event> events_;
};

#endif //MODERNCPPWEBSERVER_SERVER_EPOLLER_H_
