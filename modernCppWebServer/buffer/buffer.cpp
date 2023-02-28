//
// Created by lhm on 2023/2/25.
//

#include "buffer.h"

Buffer::Buffer(int init_buffer_size) : buffer_(init_buffer_size), read_pos_(0), write_pos_(0) {

}

// 返回可读取的内容长度，即已写入长度减已读长度为未读长度
size_t Buffer::ReadableBytes() const {
    return write_pos_ - read_pos_;
}

// 返回剩下可写入的空间大小，即缓冲区总大小减去已写入大小
size_t Buffer::WritableBytes() const {
    return buffer_.size() - write_pos_;
}

// 返回已读的可以重新写入的大小，即返回已读的位置
size_t Buffer::PrependableBytes() const {
    return read_pos_;
}

// 返回缓冲区已读内容的顶端地址，即起始指针向后移动已读长度
const char* Buffer::Peek() const {
    return BeginPtr_() + read_pos_;
}

// 检索len个字节，即已读长度向后移动增加len
void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    read_pos_ += len;
}

// 向后检索直到传入的*end指针位置
void Buffer::RetrieveUntil(const char *end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

// 清空所有缓存，将已读已写长度置为0，将缓存空间内容全清0
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}

// 检索出所有未读的数据，以string类型返回，并清空所有缓存
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

// 返回写入位置的指针，即起始指针向后移动已写长度，const版
const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + write_pos_;
}

// 返回写入位置的指针，即起始指针向后移动已写长度
char* Buffer::BeginWrite() {
    return BeginPtr_() + write_pos_;
}

// 标识向后写len个长度，即已写位置向后移动len个位置
void Buffer::HasWritten(size_t len) {
    write_pos_ += len;
}

// 向缓冲区写入字符串str
void Buffer::Append(const std::string &str) {
    // 取出string容器中的数据，再调用void Buffer::Append(const void* data, size_t len)
    Append(str.data(), str.length());
}

// 从缓存区内写入指针str指向的len长个的字节
void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

// 从缓存区内写入指针str指向的len长个的字节
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    // 确保有len个可写空间
    EnsureWritable(len);
    // 向缓冲区写入数据
    std::copy(str, str + len, BeginWrite());
    // 标识写入了len长
    HasWritten(len);
}

// 向缓存区写入另一个缓冲区内的字节
void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 确保有len个长度的可写空间，如果没有，则调用MakeSpace成员函数腾出空间
void Buffer::EnsureWritable(size_t len) {
    if (WritableBytes() < len) {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

// 从文件描述符fd中读取内容到缓冲区，并保存可能出现的错误码，返回读取的字节数
ssize_t Buffer::ReadFd(int fd, int *save_errno) {
    char buff[65535];   // 开辟一个足够大的临时缓冲区（64kb）
    struct iovec iov[2];
    const size_t writable = WritableBytes();    // 缓冲区的可写长度
    // 原作者注释
    /* 分散读，保证数据全部读完 */
    iov[0].iov_base = BeginPtr_() + write_pos_; // BeginWrite()，写入缓冲区的开始位置
    iov[0].iov_len = writable;  // 可写的长度
    // 设置临时缓冲区
    iov[1].iov_base = buff; //
    iov[1].iov_len = sizeof(buff);

    // 同时从fd中读取内容到临时缓冲区和对象自己的缓存区
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        // 如果读取失败，保存错误码
        *save_errno = errno;
    } else if (static_cast<size_t>(len) <= writable) {
        // 如果读取的字节数小于可写的缓冲区空间
        // 往后写len字节（其实在readv已经写入，这里将写入位置的标识移动）
        write_pos_ += len;
    } else {
        // 如果读取的字节数大于可写的缓冲区空间
        // 将写入位置移动到最后，调用Append从临时缓冲区中把没有写入的内容写入到缓存区
        // （Append内部调用MakeSpace_分配足够的空间，再将写位置从当前位置后移）
        write_pos_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}

// 向文件描述符fd写入缓冲区中可读的字节，并保存可能出现的错误码，返回写入的字节数
ssize_t Buffer::WriteFd(int fd, int *save_errno) {
    // 从可读位置开始，向文件描述符fd可读的字节
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if (len < 0) {
        // 如果写入失败，保存错误码，直接返回
        *save_errno = errno;
        return len;
    }
    // 可读位置后移len
    read_pos_ += len;
    return len;
}

// 返回缓冲区的起始地址字符指针
char* Buffer::BeginPtr_() {
    return &*buffer_.begin(); // vector第一个元素的地址
}

// 返回缓冲区的起始地址字符指针
const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

// 为缓冲区腾出len大小的空间
void Buffer::MakeSpace_(size_t len) {
    if (WritableBytes() + PrependableBytes() < len) {
        // 如果可写的空间加上已读的可以重新写入的空间不足len，则为缓冲区容器重新分配空间
        buffer_.resize(write_pos_ + len + 1);
    } else {
        // 获取未读的大小
        size_t readable = ReadableBytes();
        // 将未读的内容移动到缓冲区最前方，其他内容就相当于清空了
        std::copy(BeginPtr_() + read_pos_, BeginPtr_() + write_pos_, BeginPtr_());
        // 重置已读位置，已写位置
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable;
        // 移动后，确定一下，可读大小是否正确
        assert(readable == ReadableBytes());
    }
}



