//
// Created by lhm on 2023/2/25.
//

#ifndef MODERNCPPWEBSERVER_BUFFER_H
#define MODERNCPPWEBSERVER_BUFFER_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

class Buffer {
public:
  Buffer(int init_buffer_size = 1024);
  ~Buffer() = default;

  // 返回剩下可写入的空间大小，为缓冲区总大小减去已写入大小
  size_t WritableBytes() const;
  // 返回可读取的内容长度，已写入长度减已读长度为未读长度
  size_t ReadableBytes() const;
  // 返回已读的可以重新写入的大小，即返回已读的位置
  size_t PrependableBytes() const;

  // 返回缓冲区已读内容的顶端地址，为起始指针向后移动已读长度
  const char *Peek() const;
  // 确保有len个长度的可写空间，如果没有，则调用MakeSpace成员函数腾出空间
  void EnsureWritable(size_t len);
  // 标识向后写len个长度，即已写位置向后移动len个位置
  void HasWritten(size_t len);

  // 检索len个字节，即已读长度向后移动增加len
  void Retrieve(size_t len);
  // 向后检索直到传入的*end指针位置
  void RetrieveUntil(const char *end);

  // 清空所有缓存，将已读已写长度置为0，将缓存空间内容全清0
  void RetrieveAll();
  // 检索出所有未读的数据，以string类型返回，并清空所有缓存
  std::string RetrieveAllToStr();

  // 返回写入位置的指针，即起始指针向后移动已写长度，const版
  const char *BeginWriteConst() const;
  // 返回写入位置的指针，即起始指针向后移动已写长度
  char *BeginWrite();

  void Append(const std::string &str); // 向缓冲区写入字符串str
  // 从缓存区内写入指针str指向的len长个的字节
  void Append(const char *str, size_t len);
  // 从缓存区内写入指针str指向的len长个的字节
  void Append(const void *data, size_t len);
  // 向缓存区写入另一个缓冲区内的字节
  void Append(const Buffer &buff);

  // 从文件描述符fd中读取内容到缓冲区，并保存可能出现的错误码，返回读取的字节数
  ssize_t ReadFd(int fd, int *save_errno);
  // 向文件描述符写入缓冲区中可读的字节，并保存可能出现的错误码，返回写入的字节数
  ssize_t WriteFd(int fd, int *save_errno);

private:
  // 返回缓冲区的起始地址字符指针
  char *BeginPtr_();
  // 返回缓冲区的起始地址字符指针
  const char *BeginPtr_() const;
  // 为缓冲区腾出len大小的空间
  void MakeSpace_(size_t len);

  std::vector<char> buffer_; // 存放缓冲区的数据
  // 已经读取缓冲区的字节数，即下一个读取的位置下标
  std::atomic<std::size_t> read_pos_;
  // 已经读取缓冲区的字节数，即下一个读取的位置下标
  std::atomic<std::size_t> write_pos_;
};

#endif // MODERNCPPWEBSERVER_BUFFER_H
