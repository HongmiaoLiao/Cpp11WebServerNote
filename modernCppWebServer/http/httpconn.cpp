//
// Created by lhm on 2023/3/15.
//

#include "httpconn.h"

// 初始化静态变量为空
bool HttpConn::isET;
const char *HttpConn::src_dir;
std::atomic<int> HttpConn::user_count;

HttpConn::HttpConn() : fd_(-1), addr_({0}), is_close_(true) {

}

HttpConn::~HttpConn() {
  Close();
}

// 初始化连接，入参为套接字的文件描述符合客户端地址
void HttpConn::Init(int sock_fd, const sockaddr_in &addr) {
  assert(sock_fd > 0);
  // 连接用户+1
  ++user_count;
  // 设置客户地址
  addr_ = addr;
  // 设置套接字文件描述符
  fd_ = sock_fd;
  // 清空读缓重区和写缓冲区
  write_buff_.RetrieveAll();
  read_buff_.RetrieveAll();
  // 设置打开标识，写入日志
  is_close_ = false;
  LOG_INFO("Client[%d](%s:%d) in, user_count:%d", fd_, GetIP(), GetPort(), (int) user_count);
}

// 关闭一个当前用户的连接
void HttpConn::Close() {
  // 删除响应文件在内存中的映射
  response_.UnmapFile();
  if (is_close_ == false) {
    // 如果连接是开启状态，则置为关闭状态，用户数减1，关闭套接字，记下日志
    is_close_ = true;
    --user_count;
    close(fd_);
    LOG_INFO("Client[%d](%s:%d) quit, user_count:%d", fd_, GetIP(), GetPort(), (int) user_count);
  }
}

// 返回套接字文件描述符，即返回成员变量fd_
int HttpConn::GetFd() const {
  return fd_;
}

// 返回客户端地址信息，即返回成员变量addr_
sockaddr_in HttpConn::GetAddr() const {
  return addr_;
}

// 返回客户端的ip地址，即利用库函数提取地址中的ip
const char *HttpConn::GetIP() const {
  return inet_ntoa(addr_.sin_addr);
}

// 返回客户端的端口号，即从地址结构体中取出端口号
int HttpConn::GetPort() const {
  return addr_.sin_port;
}

// 将套接字的内容（即请求的内容）读入读缓存区，并设置错误码（如果出错）
ssize_t HttpConn::Read(int *save_errno) {
  ssize_t len = -1;
  do {
    // 循环读取，直到读取的长度小于0（读取完毕）
    len = read_buff_.ReadFd(fd_, save_errno);
    if (len <= 0) {
      break;
    }
  } while (isET);
  return len;
}

// 将写缓存区的内容以及映射到内存的请求内容（如果有的话）从套接字发送给客户端
ssize_t HttpConn::Write(int *save_errno) {
  ssize_t len = -1;
  do {
    // 循环向套接字写入内容
    len = writev(fd_, iov_, iov_cnt_);
    if (len <= 0) {
      *save_errno = errno;
      break;
    }
    if (iov_[0].iov_len + iov_[1].iov_len == 0) {
      // 如果要发送的长度为0
      break; /* 原作者注释， 传输结束 */
    } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
      // 如果写入长度大于第一个通道（写缓冲区写入长度），说明写缓冲区写完。
      // 将第二个通道（响应文件在内存的映射）的起点向后移动到还未传输的地方
      iov_[1].iov_base = (uint8_t *) iov_[1].iov_base + (len - iov_[0].iov_len);
      // 将第二个通道待传输的内容长度减去已经传输的，即记下还要传输的内容长度
      iov_[1].iov_len -= (len - iov_[0].iov_len);
      // 将第一个通道的传输长度置为0，清空写缓冲区
      if (iov_[0].iov_len) {
        write_buff_.RetrieveAll();
        iov_[0].iov_len = 0;
      }
    } else {
      // 如果写入长度小于等于第一个通道（写缓冲区写入长度），说明写缓冲区还没写完。
      // 第一个通道位置向后移动已写的长度
      iov_[0].iov_base = (uint8_t *) iov_[0].iov_base + len;
      // 第一个通道待写的长度减去已写的长度
      iov_[0].iov_len -= len;
      // 写缓冲区已读标识向后移动已写长度
      write_buff_.Retrieve(len);
    }
  } while (ToWriteBytes()> 0);
  // } while (isET || ToWriteBytes() > 10240);
  return len;
}

// 处理http请求，并根据请求处理好响应内容，将待写入套接字的通道接入对应位置（第一个为写缓冲区，第二个为响应内容内存的映射
bool HttpConn::Process() {
  if (request_.State() == HttpRequest::kFinish) {
    // 如果请求处于结束（上一个请求结束）状态，初始化请求对象
    request_.Init();
  }
  if (read_buff_.ReadableBytes() <= 0) {
    // 如果读缓冲区为空，说明没有从套接字读入数据，返回false
    return false;
  }
  // 解析HTTP请求
  HttpRequest::HttpCode process_state = request_.Parse(read_buff_);
  if (process_state == HttpRequest::kGetRequest) {
    // 如果是get请求，用解析出的请求路径初始化响应（src_dir在服务器主类中初始化）
    LOG_DEBUG("request path %s", request_.Path().c_str());
    response_.Init(src_dir, request_.Path(), request_.IsKeepAlive(), 200);
  } else if (process_state == HttpRequest::kNoRequest) {
    // 如果解析出没有请求，直接返回
    return false;
  } else {
    // 否则用400错误号初始化响应对象
    response_.Init(src_dir, request_.Path(), false, 400);
  }
  // 向写缓冲区写入响应内容
  response_.MakeResponse(write_buff_);
  // 将通道1指向写缓冲区顶部（待读取的位置）
  iov_[0].iov_base = const_cast<char *>(write_buff_.Peek());
  // 将写入内容长度置为写缓冲区可读内容
  iov_[0].iov_len = write_buff_.ReadableBytes();
  iov_cnt_ = 1;

  if (response_.FileLen() > 0 && response_.File()) {
    // 如果响应的文件长度大于0，将通道2指向响应的文件在内存映射的起始位置，长度置为文件大小，并将通道数置为2
    iov_[1].iov_base = response_.File();
    iov_[1].iov_len = response_.FileLen();
    iov_cnt_ = 2;
  }
  return true;
}










