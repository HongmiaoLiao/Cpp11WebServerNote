//
// Created by lhm on 2023/2/25.
//

#ifndef MODERNCPPWEBSERVER_BUFFER_H
#define MODERNCPPWEBSERVER_BUFFER_H

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include <atomic>
#include <cassert>
#include <string>

class Buffer {
public:
    Buffer(int init_buffer_size = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;   // 返回剩下可写入的空间大小，为缓冲区总大小减去已写入大小
    size_t ReadableBytes() const;   // 返回可读取的内容长度，已写入长度减已读长度为未读长度
    size_t PrependableBytes() const;    // 返回已读的可以重新写入的大小，即返回已读的位置

    const char* Peek() const;   // 返回缓冲区已读内容的顶端地址，为起始指针向后移动已读长度
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);  // 检索len个字节，即已读长度向后移动增加len
    void RetrieveUntil(const char* end);    // 向后检索直到传入的*end指针位置

    void RetrieveAll(); // 清空所有缓存，将已读已写长度置为0，将缓存空间内容全清0
    std::string RetrieveAllToStr(); // 检索出所有未读的数据，以string类型返回，并清空所有缓存

    const char* BeginWriteConst() const;    // 返回写入位置的指针，即起始指针向后移动已写长度，const版
    char* BeginWrite(); // 返回写入位置的指针，即起始指针向后移动已写长度

    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* save_errno);
    ssize_t WriteFd(int fd, int* save_errno);

private:
    char* BeginPtr_();  // 返回缓冲区的起始地址字符指针
    const char* BeginPtr_() const;  // 返回缓冲区的起始地址字符指针
    void MakeSpace_(size_t len);    // 为缓冲区腾出len大小的空间

    std::vector<char> buffer_;  // 存放缓冲区的数据
    std::atomic<std::size_t> read_pos_; // 已经读取缓冲区的字节数，即下一个读取的位置下标
    std::atomic<std::size_t> write_pos_;    // 已经读取缓冲区的字节数，即下一个读取的位置下标
};

#endif //MODERNCPPWEBSERVER_BUFFER_H
