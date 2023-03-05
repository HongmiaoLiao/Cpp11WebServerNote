//
// Created by lhm on 2023/3/3.
//

#ifndef MODERNCPPWEBSERVER_LOG_LOG_H_
#define MODERNCPPWEBSERVER_LOG_LOG_H_

#include <mutex>
#include <string>
#include <thread>
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
  void init(int level, const char *path = "./log",
            const char *suffix = "./log",
            int max_queue_capacity = 1024);

  // 创建静态日志对象，获取日志对象的指针，单例模式获取对象的方法
  static Log *Instance();
  // 调用异步写，将队列内容全部写入文件
  static void FlushLogThread();

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
  static const int kLogPathLen = 256;
  static const int kLogNameLen = 256;
  static const int kMaxLines = 50000;

  const char *path_;
  const char *suffix_;

  int max_lines_;

  int line_count_;
  int today_;

  bool is_open_;

  Buffer buff_;
  int level_;
  bool is_async_;

  std::FILE *fp_;
  std::unique_ptr<BlockDeque<std::string>> deque_;
  std::unique_ptr<std::thread> write_thread_;
  std::mutex mtx_;
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
