#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <pthread.h>
#include "block_queue.h"
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>

using namespace std;

class Log {
public:
    // C++11以后，使用局部变量懒汉不用加锁
    static Log* getInstance() {
        static Log instance;
        return &instance;
    }

    // 异步写日志公有方法，调用私有方法asyncWriteLog
    static void* flushLogThread(void* args) {
        Log::getInstance()->asyncWriteLog();
    }

    // 可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char* fileName, int closeLog, int logBufsize=8192, int splitLines=5000000, int maxQueueSize=0);

    // 将输出内容按照标准格式整理
    void writeLog(int level, const char* format, ...);
    
    // 强制刷新缓冲区
    void flush(void);

private:
    Log();
    virtual ~Log();

    // 异步写日志方法
    void* asyncWriteLog() {
        string singleLog;
        // 从阻塞队列中取出一个日志string， 写入文件
        while (logQueue->pop(singleLog)) {
            mutex.lock();
            fputs(singleLog.c_str(), fp);
            mutex.unlock();
        }
    }

    char dirName[128];  // 路径名
    char logName[128];  // log文件名
    int splitLines; //日志最大行数
    int logBufsize; // 日志缓冲区大小
    long long count;    // 日志行数记录
    int today;  // 因为按天分类，记录当前时间是那一天
    FILE* fp;   // 打开log的文件指针
    char* buf;   // 要输出的内容
    BlockQueue<string>* logQueue;   // 阻塞队列
    bool isAsync;   // 是否同步标志位
    Locker mutex;
    int closeLog;   // 关闭日志
};

// 这四个宏定义在其他文件中使用，主要用于不同类型的日志输出
#define LOG_DEBUG(format, ...) if(closeLog == 0) {Log::getInstance()->writeLog(0, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_INFO(format, ...) if(closeLog == 0) {Log::getInstance()->writeLog(1, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_WARN(format, ...) if(closeLog == 0) {Log::getInstance()->writeLog(2, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_ERROR(format, ...) if(closeLog == 0) {Log::getInstance()->writeLog(3, format, ##__VA_ARGS__); Log::getInstance()->flush();}

#endif