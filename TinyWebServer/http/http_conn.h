#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"


class HttpConn {
public:
    static const int FILENAME_LEN = 200;    // 读取文件名称realFile大小
    static const int READ_BUFFER_SIZE = 2048;   // 设置读缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024;  // 设置写缓存区大小

    int timerFlag;
    int improve;

    static int epollfd;
    static int userCount;
    MYSQL* mysql;
    int state; // 读为0，写为1
    // 报文的请求方法，本项目只用到GET和POST
    enum METHOD {
        GET = 0,    //注意着是逗号
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    // 主状态机的状态
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    // 报文解析的结果
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_RQEUEST,
        FILE_REQUEST,
        INTERNAL_EROR,
        CLOSED_CONNECTION
    };
    // 从状态机的状态
    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    HttpConn() {}
    ~HttpConn() {}
    // 初始化套接字地址，函数内部会调用私有方法init
    void init(int sockfd, const sockaddr_in& addr, char*, int, int, string user, string password, string sqlName);
    void closeConn(bool realClose = true);  // 关闭http连接
    void process();
    bool readOnce();   // 读取浏览器端发来的全部数据
    bool write();       // 响应报文写入函数
    sockaddr_in* getAddress() {
        return &address;
    }
    void initMysqlResult(ConnectionPool *connPool); // 同步线程初始化数据库读取表
    
private:
    int sockfd;
    sockaddr_in address;
    char readBuf[READ_BUFFER_SIZE]; // 存储读取的请求报文数据
    int readIdx;    // 缓冲区中readBuf中数据的最后一个字节的下一个位置
    int checkedIdx; // readBuf读取的位置
    int startLine;  // readBuf中已经解析的字符个数
    char writeBuf[WRITE_BUFFER_SIZE];   // 存储发出的响应报文数据
    int writeIdx;   // 知识buffer中的长度
    CHECK_STATE checkState; // 主状态机的状态
    METHOD method;  // 请求方法
    // 以下文解析请求报文中对应的六个变量
    char realFile[FILENAME_LEN];    // 存储读取文件的名称
    char* url;
    char* version;
    char* host;
    int contentLength;
    bool linger;

    char* fileAddress;   // 读取服务器上的文件地址
    struct stat fileStat;   
    struct iovec m_iv[2];   //io向量机制iovec
    int ivCount;
    int cgi;    // 是否启用的POST
    char* str;  //存储请求头数据
    int bytesToSend;    // 剩余发送字节数
    int bytesHaveSend;  // 已经发送字节数
    char* docRoot;

    map<string, string>users;
    int trigMode;
    int closeLog;

    char sqlUser[100];
    char sqlPassword[100];
    char sqlName[100];

    void init();    
    HTTP_CODE processRead();    // 从readBuf读取，并处理请求报文
    bool processWrite(HTTP_CODE ret);   // 向writeBuf写入响应报文数据
    HTTP_CODE parseRequestLine(char* text); // 主状态机解析报文中的请求行数据 
    HTTP_CODE parseHeaders(char* text); // 主状态机解析报文中的请求头数据
    HTTP_CODE parseContent(char* text); // 主状态机解析报文中的请求内容
    HTTP_CODE doRequest();  // 生成响应报文
    char* getLine() {
        return readBuf + startLine; // startLine是已经解析的字符
    }   // getLine用于将指针向后便宜，指向未处理的字符
    LINE_STATUS parseLine();    // 从状态机读取一行，分析是请求报文的哪一部分
    void unmap();
    bool addResponse(const char* format, ...);  // 根据响应报文，生成对应8个部分，以下函数均有doRequest调用
    bool addContent(const char* content);
    bool addStatusLine(int status, const char* title);
    bool addHeaders(int contentLength);
    bool addContentType();
    bool addContentLength(int contentLength);
    bool addLinger();
    bool addBlankLine();

};


#endif