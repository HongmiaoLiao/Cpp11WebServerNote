#include "config.h"

int main(int argc, char* argv[]) {
    // 需要修改的数据库信息，登录名，密码，库名
    string user = "root";
    string passwd = "L248132240";
    string databasename = "tinywebserver";

    // 命令行解析
    Config config;
    config.parseArg(argc, argv);

    WebServer server;
    // 初始化
    server.init(config.port, user, passwd, databasename, config.logWrite,
                config.optLinger, config.trigMode, config.sqlNum, config.threadNum,
                config.closeLog, config.actorModel);
    
    // 日志
    server.setLogWrite();
    // 数据库
    server.sqlPool();
    // 线程池
    server.threadPool();
    // 触发模式
    server.setTrigMode();
    // 监听
    server.eventListen();
    // 运行
    server.eventLoop();

    return 0;
}