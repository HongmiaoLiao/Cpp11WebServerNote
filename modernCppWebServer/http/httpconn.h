//
// Created by lhm on 2023/3/15.
//

#ifndef MODERNCPPWEBSERVER_HTTP_HTTPCONN_H_
#define MODERNCPPWEBSERVER_HTTP_HTTPCONN_H_

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
 public:
  HttpConn();

  ~HttpConn();

  // 初始化连接，入参为套接字的文件描述符合客户端地址
  void Init(int sock_fd, const sockaddr_in &addr);

  // 将套接字的内容（即请求的内容）读入读缓存区，并设置错误码（如果出错）
  ssize_t Read(int *save_errno);

  ssize_t Write(int *save_errno);

  // 关闭一个当前用户的连接
  void Close();

  // 返回套接字文件描述符，即返回成员变量fd_
  int GetFd() const;

  // 返回客户端的端口号，即从地址结构体中取出端口号
  int GetPort() const;

  // 返回客户端的ip地址，即利用库函数提取地址中的ip
  const char *GetIP() const;

  // 返回客户端地址信息，即返回成员变量addr_
  sockaddr_in GetAddr() const;

  // 处理http请求，并根据请求处理好响应内容，将待写入套接字的通道接入对应位置（第一个为写缓冲区，第二个为响应内容内存的映射
  bool Process();

  // 待发送给套接字的字节数，即两个通道中待写入的长度
  int ToWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
  }

  // 从请求中获取该请求是否是长连接
  bool IsKeepAlive() const {
    return request_.IsKeepAlive();
  }

  static bool isET;
  static const char *src_dir;
  static std::atomic<int> user_count;

 private:
  int fd_;
  struct sockaddr_in addr_;

  bool is_close_;

  int iov_cnt_;
  struct iovec iov_[2];

  Buffer read_buff_;
  Buffer write_buff_;

  HttpRequest request_;
  HttpResponse response_;
};

#endif //MODERNCPPWEBSERVER_HTTP_HTTPCONN_H_
