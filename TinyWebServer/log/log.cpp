#include "log.h"


using namespace std;

Log::Log() {
    count = 0;
    isAsync = false;
}

Log::~Log() {
    if (fp != NULL) {
        fclose(fp);
    }
}

// 异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char* fileName, int closeLog, int logBufsize, int splitLines, int maxQueueSize) {
    // 如果设置了maxQueueSize,则设置为异步
    if (maxQueueSize >= 1) {
        // 设置写入方式flag
        this->isAsync = true;
        // 创建并设置阻塞队列长度
        this->logQueue = new BlockQueue<string>(maxQueueSize);
        pthread_t tid;
        // flushLogThread为回调函数，这里表示创建线程异步写日志
        pthread_create(&tid, NULL, flushLogThread, NULL);
    }

    // 输出内容的长度
    this->closeLog = closeLog;
    this->logBufsize = logBufsize;
    this->buf = new char[logBufsize];
    memset(buf, '\0', logBufsize);

    // 日志的对大行数
    this->splitLines = splitLines;

    time_t t = time(NULL);
    struct tm* sysTm = localtime(&t);
    struct tm myTm = *sysTm;

    // 从后往前找到第一个/的位置
    const char* p = strrchr(fileName, '/');
    char logFullName[256] = {0};

    // 相当于自定义日志名
    // 若输入的文件名没有/，则直接将时间+文件名作为日志名
    if (p == NULL) {
        snprintf(logFullName, 255, "%d_%02d_%02d_%s", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, fileName);
    } else {
        // 将/的位置往后移动一个位置，然后复制到logname 中
        // p - fileName + 1是文件所在路径文件夹的长度
        // dirName相当于./
        strcpy(logName, p + 1);
        strncpy(dirName, fileName, p - fileName + 1);

        //后面的参数跟
        snprintf(logFullName, 255, "%s%d_%02d_%02d_%s", dirName, myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, logName);
    }

    today = myTm.tm_mday;
    fp = fopen(logFullName, "a");
    if (fp == NULL) {
        return false;
    }
    return true;
}

void Log::writeLog(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm myTm = *sys_tm;
    char s[16] = {0};
    // 日志分级
    switch (level) {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[errno]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    mutex.lock();
    // 更新现有行数
    ++count;
    // 日志不是今天写入或写入的日志行数是最大行的倍数
    if (today != myTm.tm_mday || count % splitLines == 0) { // everyday log
        char newLog[256] = {0};
        fflush(fp);
        fclose(fp);
        char tail[16] = {0};
        // 格式化日志名中的时间部分
        snprintf(tail, 16, "%d_%02d_%02d_", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday);

        // 如果时间不是今天，则创建今天的日志，更新today和count
        if (today != myTm.tm_mday) {
            snprintf(newLog, 255, "%s%s%s", dirName, tail, logName);
            today = myTm.tm_mday;
            count = 0;
        } else {
            // 超过了最大行，在之前的日志名基础上加后缀，count/splitLines
            snprintf(newLog, 255, "%s%s%s.%lld", dirName, tail, logName, count / splitLines);
        }
        fp = fopen(newLog, "a");
    }

    mutex.unlock();

    va_list valst;
    // 将传入的format参数赋值给valst， 便于格式化输出
    va_start(valst, format);

    string logStr;
    mutex.lock();
    // 写入格式： 时间 + 内容
    // 时间格式化，snprintf成功返回写字符的总数，其中不包括结尾的null字符
    int n = snprintf(buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday,
                     myTm.tm_hour, myTm.tm_min, myTm.tm_sec, now.tv_usec, s);

    // 内容格式化，用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数（不包含终止符）
    int m = vsnprintf(buf + n, logBufsize - 1, format, valst);
    buf[n+m] = '\n';
    buf[n+m+1] = '\0';

    logStr = buf;
    mutex.unlock();

    // 若isAsync为true表示异步，默认为同步
    // 若异步，则将日志信息加入阻塞队列，同步则加锁向文件写
    if (isAsync && !logQueue->full()) {
        logQueue->push(logStr);
    } else {
        mutex.lock();
        fputs(logStr.c_str(), fp);
        mutex.unlock();
    }
    va_end(valst);
}

void Log::flush(void) {
    mutex.lock();
    // 强制刷新写入流缓冲区
    fflush(fp);
    mutex.unlock();
}