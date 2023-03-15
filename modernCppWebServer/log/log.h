//
// Created by lhm on 2023/3/3.
//

#ifndef MODERNCPPWEBSERVER_LOG_LOG_H_
#define MODERNCPPWEBSERVER_LOG_LOG_H_

#include <mutex>
#include <string>
#include <thread>
// #include <ctime>
#include <sys/time.h>
#include <cstring>
#include <cstdarg>
#include <cassert>
#include <sys/stat.h>
#include <cstdio>
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
 public:
  // 执行日志系统的初始化
  void Init(int level, const char *path = "./log",
            const char *suffix = "./log",
            int max_queue_capacity = 1024);

  // 创建静态日志对象，获取日志对象的指针，单例模式获取对象的方法
  static Log *Instance();
  // 调用异步写，将队列内容全部写入文件
  static void FlushLogThread();

  // 根据初始化的信息和当前时间，实现对可变参数内容的写入操作
  void Write(int level, const char *format, ...);
  // 将未写入文件的日志内容写入
  void Flush();

  // 获取日志等级，即获取私有成员变量日志标识
  int GetLevel();
  // 设置日志等级，即设置私有成员变量日志标识
  void SetLevel(int level);
  // 判断日志是否关闭，即然后是否关闭的成员变量标识
  bool IsOpen() {
    return is_open_;
  }

 private:
  Log();
  // 根据日志等级，向缓冲区放入相应的日志头部信息字符串
  void AppendLogLevelTitle_(int level);
  virtual ~Log();

  // 异步写，将阻塞队列的内容全部写入文件
  void AsyncWrite_();

 private:
  static const int kLogPathLen = 256; // 常量，表示最大日志文件路径的长度为256（没有用上）
  static const int kLogNameLen = 256; // 常量，表示日志文件名的最长名字为256
  static const int kMaxLines = 50000; // 常量，表示日志文件中最大行数为50000

  const char *path_;  // 字符串常量的指针，指向日志文件路径
  const char *suffix_;  // 字符串常量的指针，指向日志文件后缀名

  int max_lines_; // 未使用

  int line_count_;  // 表示当前日志的行数
  int today_; // 表示当前为本月第几天

  bool is_open_;  // 表示日志系统是否开启

  Buffer buff_; // 日志内容缓冲区
  int level_; // 日志等级，一共0，1，2，3四个等级，对应debug,info,warn,error
  bool is_async_; // 表示是否异步写入

  std::FILE *fp_; // 文件类型指针，用于对日志文件的写入
  std::unique_ptr<BlockDeque<std::string>> deque_;  // 独占指针，指向存放日志信息的阻塞队列
  std::unique_ptr<std::thread> write_thread_; // 独占指针，指向将日志信息写入文件的线程
  std::mutex mtx_;  // 互斥锁，用于日志文件的互斥写入
};

#define LOG_BASE(level, format, ...) \
  do {\
    Log* log = Log::Instance();      \
    if (log->IsOpen() && log->GetLevel() <= level) { \
      log->Write(level, format, ##__VA_ARGS__);      \
      log->Flush(); \
    }  \
  } while (0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //MODERNCPPWEBSERVER_LOG_LOG_H_
