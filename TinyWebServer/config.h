#ifndef CONFIG_H
#define OCNFIG_H

#include "webserver.h"

using namespace std;

class Config {
public:
    Config();
    ~Config() {

    }

    void parseArg(int argc, char* argv[]);

    // 端口号
    int port;
    // 日志写入方式
    int logWrite;
    // 触发组合模式
    int trigMode;
    // listenfd触发模式
    int listenTrigMode;
    // connfd触发模式
    int connTrigMode;
    // 优雅关闭连接
    int optLinger;
    // 数据库连接池数量
    int sqlNum;
    // 线程池内的线程数量
    int threadNum;
    // 是否关闭日志
    int closeLog;
    // 并发模式选择
    int actorModel;

};

#endif