//
// Created by lhm on 2023/3/3.
//

#include "log.h"

Log::Log()
    : line_count_(0),
      is_async_(false),
      write_thread_(nullptr),
      deque_(nullptr),
      today_(0),
      fp_(nullptr) {

}
Log::~Log() {
  if (write_thread_ && write_thread_->joinable()) {
    // 如果写线程指针不为空且线程还在执行
    // deque_->Close();
    while (!deque_->IsEmpty()) {
      // 阻塞队列不为空的话，将消费者线程唤醒使其消费完队列内容
      deque_->Flush();
    }
    deque_->Close();  // 关闭阻塞队列
    // 阻塞线程，使线程对象可以安全销毁
    write_thread_->join();
  }
  if (fp_) {
    // 如果文件指针不为空
    std::lock_guard<std::mutex> locker(mtx_);
    // 上锁后，写入未写入内容
    Flush();
    // 关闭文件指针
    std::fclose(fp_);
  }
}

// 获取日志等级，即获取私有成员变量日志标识
int Log::GetLevel() {
  std::lock_guard<std::mutex> locker(mtx_);
  return level_;
}

// 设置日志等级，即设置私有成员变量日志标识
void Log::SetLevel(int level) {
  std::lock_guard<std::mutex> locker(mtx_);
  level_ = level;
}

void Log::init(int level, const char *path, const char *suffix,
               int max_queue_capacity) {
  is_open_ = true;
  level_ = level;
  if (max_queue_capacity > 0) {
    is_async_ = true;
    if (!deque_) {
      std::unique_ptr<BlockDeque<std::string>> new_deque(new BlockDeque<std::string>);
      deque_ = std::move(new_deque);

      std::unique_ptr<std::thread> new_thread(new std::thread(FlushLogThread));
      write_thread_ = std::move(new_thread);
    }
  } else {
    is_async_ = false;
  }

  line_count_ = 0;

  time_t timer = time(nullptr);
  struct tm *sys_time = localtime(&timer);
  struct tm t = *sys_time;
  path_ = path;
  suffix_ = suffix;
  char file_name[kLogNameLen] = {0};
  snprintf(file_name, kLogNameLen - 1, "%s/%04d_%02d_%02d%s", path_,
           t.tm_year + 1990, t.tm_mon + 1, t.tm_mday, suffix_);
  today_ = t.tm_mday;

  {
    std::lock_guard<std::mutex> locker(mtx_);
    buff_.RetrieveAll();
    if (fp_) {
      Flush();
      fclose(fp_);
    }
    fp_ = fopen(file_name, "a");
    if (fp_ == nullptr) {
      mkdir(path_, 0777);
      fp_ = fopen(file_name, "a");
    }
    assert(fp_ != nullptr);
  }
}

void Log::Write(int level, const char *format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t t_sec = now.tv_sec;
  struct tm *sys_time = localtime(&t_sec);
  struct tm t = *sys_time;
  va_list valist;

  // 原作者注释
  /* 日志日期 日志行数 */
  if (today_ != t.tm_mday || (line_count_ && (line_count_ % kMaxLines == 0))) {
    // std::unique_lock<std::mutex> locker(mtx_);
    // locker.unlock();

    char new_file[kLogNameLen];
    char tail[36] = {0};
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    if (today_ != t.tm_mday) {
      snprintf(new_file, kLogNameLen - 72, "%s%s%s", path_, tail, suffix_);
      today_ = t.tm_mday;
      line_count_ = 0;
    } else {
      snprintf(new_file, kLogNameLen - 72, "%s%s-%d%s", path_, tail, (line_count_ / kMaxLines), suffix_);
    }

    // locker.lock();
    // 觉得没必要建立锁后就解锁，到用时上锁，可以在这里用lock_guard锁，作用域结束时释放
    std::lock_guard<std::mutex> locker(mtx_);
    Flush();
    fclose(fp_);
    fp_ = fopen(new_file, "a");
    assert(fp_ != nullptr);
  }

  {
    // std::unique_lock<std::mutex> locker(mtx_); // 这里unique_lock和lock_guard都可以吧
    std::lock_guard<std::mutex> locker(mtx_);
    ++line_count_;
    int n = snprintf(buff_.BeginWrite(), 128, "%d_%02d_%02d %02d:%02d:%02d.%06ld ", t.tm_year + 1900,
                     t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
    buff_.HasWritten(n);
    AppendLogLevelTitle_(level);

    va_start(valist, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, valist);
    va_end(valist);

    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);

    if (is_async_ && deque_ && !deque_->IsFull()) {
      deque_->PushBack(buff_.RetrieveAllToStr());
    } else {
      fputs(buff_.Peek(), fp_);
    }
    buff_.RetrieveAll();
  }
}

// 根据日志等级，向缓冲区放入相应的日志头部信息字符串
void Log::AppendLogLevelTitle_(int level) {
  switch (level) {
    case 0:
      buff_.Append("[debug]: ", 9);
      break;
    case 1:
      buff_.Append("[info] : ", 9);
      break;
    case 2:
      buff_.Append("[warn] : ", 9);
      break;
    case 3:
      buff_.Append("[error]: ", 9);
      break;
    default:
      buff_.Append("[info] : ", 9);
      break;
  }
}

// 将未写入文件的日志内容写入
void Log::Flush() {
  if (is_async_) {
    // 如果是异步的，唤醒一个消费者线程
    deque_->Flush();
  }
  //
  std::fflush(fp_);
}

// 异步写，将阻塞队列的内容全部写入文件
void Log::AsyncWrite_() {
  std::string str;
  while (deque_->Pop(str)) {
    // 弹出阻塞队列内的字符串，直到队列为空
    std::lock_guard<std::mutex> locker(mtx_); // 互斥写
    fputs(str.c_str(), fp_);
  }
}

// 创建静态日志对象，获取日志对象的指针，单例模式获取对象的方法
Log *Log::Instance() {
  static Log inst;
  return &inst;
}

// 调用异步写，将队列内容全部写入文件
void Log::FlushLogThread() {
  Log::Instance()->AsyncWrite_();
}







