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

// 执行日志系统的初始化
void Log::Init(int level, const char *path, const char *suffix,
               int max_queue_capacity) {
  is_open_ = true;
  level_ = level;
  if (max_queue_capacity > 0) {
    is_async_ = true;
    if (!deque_) {
      // 如果阻塞队列指针为空，即还未创建，则用独占指针指向一个新建的存放string的阻塞队列
      std::unique_ptr<BlockDeque<std::string>> new_deque(new BlockDeque<std::string>);
      deque_ = std::move(new_deque);

      // 新建一个独占指针指向执行异步写任务的线程对象，然后将其move给私有成员的deque;线程实例化后就开始执行
      std::unique_ptr<std::thread> new_thread(new std::thread(FlushLogThread));
      write_thread_ = std::move(new_thread);
    }
  } else {
    is_async_ = false;
  }

  line_count_ = 0;

  // 获取当前时间（当前unix时间戳）
  time_t timer = time(nullptr);
  // 将时间戳格式转换为本地时区对应的时间
  struct tm *sys_time = localtime(&timer);
  struct tm t = *sys_time;  // 这里用t替换指针，应该是方便操作把
  path_ = path;
  suffix_ = suffix;
  // 时间和路径、后缀名拼接作为日志文件名
  char file_name[kLogNameLen] = {0};
  std::snprintf(file_name, kLogNameLen - 1, "%s/%04d_%02d_%02d%s", path_,
           t.tm_year + 1990, t.tm_mon + 1, t.tm_mday, suffix_);
  today_ = t.tm_mday;

  {
    // 作用域内上锁
    std::lock_guard<std::mutex> locker(mtx_);
    buff_.RetrieveAll();
    if (fp_) {
      // 如果文件指针没有关闭，先将其内容写入后关闭，再重新打开
      Flush();
      std::fclose(fp_);
    }
    fp_ = std::fopen(file_name, "a");
    if (fp_ == nullptr) {
      // 如果打开文件失败，可能是不存在目录，则新建目录文件夹，再次打开
      mkdir(path_, 0777);
      fp_ = std::fopen(file_name, "a");
    }
    // 确保文件已经打开
    assert(fp_ != nullptr);
  }
}

void Log::Write(int level, const char *format, ...) {
  struct timeval now = {0, 0};
  // 获取现在的时间戳，now里面是1970以来的秒和毫秒
  gettimeofday(&now, nullptr);
  time_t t_sec = now.tv_sec;
  // 获取当前日期和时间
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
      // 如果日志时间步等于当前时间，新建当前时间日志
      snprintf(new_file, kLogNameLen - 72, "%s/%s%s", path_, tail, suffix_);
      today_ = t.tm_mday; // 更新日志时间
      line_count_ = 0;  // 设置写入行数为0
    } else {
      // (line_count_ / kMaxLines)表示的是今天第几个日志文件，相当于同名文件添加日志
      snprintf(new_file, kLogNameLen - 72, "%s/%s-%d%s", path_, tail, (line_count_ / kMaxLines), suffix_);
    }

    // locker.lock();
    // 觉得没必要建立锁后就解锁，到用时上锁，可以在这里用lock_guard锁，作用域结束时释放
    std::lock_guard<std::mutex> locker(mtx_);
    Flush();
    std::fclose(fp_);
    fp_ = std::fopen(new_file, "a");
    assert(fp_ != nullptr);
  }

  {
    // std::unique_lock<std::mutex> locker(mtx_); // 这里unique_lock和lock_guard都可以吧
    std::lock_guard<std::mutex> locker(mtx_);
    ++line_count_;
    // 将当前时间信息写入缓冲区
    int n = snprintf(buff_.BeginWrite(), 128, "%d_%02d_%02d %02d:%02d:%02d.%06ld ", t.tm_year + 1900,
                     t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
    buff_.HasWritten(n);  // 更新缓冲区已写位置
    AppendLogLevelTitle_(level);  // 将日志等级写入缓冲区

    // 将valist的信息（也就是Write( format之后的...参数)，写入到缓冲区
    va_start(valist, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, valist);
    va_end(valist);

    // 标识已写长度
    buff_.HasWritten(m);
    // 添加换行
    buff_.Append("\n\0", 2);

    if (is_async_ && deque_ && !deque_->IsFull()) {
      //  如果是异步写，则将缓冲区的内容转换为string形式作为阻塞队列的一项，有异步写线程从阻塞队列读取（消费）字符串写入文件
      deque_->PushBack(buff_.RetrieveAllToStr());
    } else {
      // 否则直接将缓冲区内容写入文件
      std::fputs(buff_.Peek(), fp_);
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
  // 将未写入内容全部写入
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







