#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <mysql/mysql.h>

#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"
#include "./timer/lst_timer.h"
#include "./log/log.h"

const int MAX_FD = 65536;   // 最大文件描述符
const int MAX_EVENT_NUMBER = 10000; // 最大事件数
const int TIMESLOT = 5;     // 最小超时单位

class WebServer {
public:
    WebServer();
    ~WebServer();

    void init(int port, string user, string passWord, string databaseName,
              int logWrite, int optLinger, int tirgmode, int sqlNum,
              int threadNum, int closeLog, int actorMode);
    
    void threadPool();
    void sqlPool();
    void setLogWrite();
    void setTrigMode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in clientAddress);
    void adjustTimer(UtilTimer* timer);
    void dealTimer(UtilTimer* timer, int sockfd);
    bool dealClinetData();
    bool dealWithSignal(bool& timeout, bool& stopServer);
    void dealWithRead(int sockfd);
    void dealWithWrite(int sockfd);

    // 基础
    int port;
    char *root;
    int logWrite;
    int closeLog;
    int actorModel;

    int pipefd[2];
    int epollfd;
    HttpConn* users;

    // 数据库相关
    ConnectionPool* connPool;
    string user;        // 登陆数据库用户名
    string passWord;    // 登陆数据库密码
    string databaseName;    // 使用数据库名
    int sqlNum;

    // 线程池相关
    ThreadPool<HttpConn>* pool;
    int threadNum;

    // epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int listenFd;
    int optLinger;
    int trigMode;
    int listenTrigMode;
    int connTrigMode;

    // 定时器相关
    clientData *usersTimer;
    Utils utils;
    
};






#endif