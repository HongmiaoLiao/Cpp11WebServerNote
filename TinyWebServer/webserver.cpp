#include "webserver.h"

WebServer::WebServer() {
    // HttpConn类对象
    users = new HttpConn[MAX_FD];

    // root文件夹路径
    char serverPath[200];
    getcwd(serverPath, 200);
    char root[6] = "/root";
    this->root = (char* ) malloc(strlen(serverPath) + strlen(root) + 1);
    strcpy(this->root, serverPath);
    strcat(this->root, root);

    // 定时器
    usersTimer = new clientData[MAX_FD];
}

WebServer::~WebServer() {
    close(epollfd);
    close(listenFd);
    close(pipefd[1]);
    close(pipefd[0]);
    delete[] users;
    delete[] usersTimer;    // 关闭定时器！
    delete[] pool;
}

void WebServer::init(int port, string user, string password, string databaseName, int logWrite, int optLinger, 
                    int trigMode, int sqlNum, int threadNum, int closeLog, int actorMode) {
    
    // 这里应该用:port(port)这样更好吧
    this->port = port;
    this->user = user;
    this->passWord = password;
    this->databaseName = databaseName;
    this->sqlNum = sqlNum;
    this->threadNum = threadNum;
    this->logWrite = logWrite;
    this->optLinger = optLinger;
    this->trigMode = trigMode;
    this->closeLog = closeLog;
    actorMode = actorMode;

}

void WebServer::setTrigMode() {
    // LT + LT
    if (trigMode == 0) {
        listenTrigMode = 0;
        connTrigMode = 0;
    } else if (trigMode == 1) {
        // LT + ET
        listenTrigMode = 0;
        connTrigMode = 1;
    } else if (trigMode == 2) {
        // ET + LT
        listenTrigMode = 1;
        connTrigMode = 0;
    } else if (trigMode == 3) {
        // ET + ET
        listenTrigMode = 1;
        connTrigMode = 1;
    }
}

void WebServer::setLogWrite() {
    if (closeLog == 0) {
        // 初始化日志
        if (logWrite == 1) {
            Log::getInstance()->init("./ServerLog", closeLog, 2000, 800000, 800);
        } else {
            Log::getInstance()->init("./ServerLog", closeLog, 2000, 800000, 0);
        }
    }
}

void WebServer::sqlPool() {
    // 初始化数据库连接池
    connPool = ConnectionPool::getInstance();
    connPool->init("localhost", user, passWord, databaseName, 3306, sqlNum, closeLog);

    // 初始化数据库读取表
    users->initMysqlResult(connPool);
}

void WebServer::threadPool() {
    // 线程池
    pool = new ThreadPool<HttpConn>(actorModel, connPool, threadNum);
}

void WebServer::eventListen() {
    // 网络编程基础步骤
    listenFd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenFd >= 0);

    // 优雅关闭连接
    if (optLinger == 0) {
        struct linger tmp = {0, 1};
        setsockopt(listenFd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    } else if (optLinger == 1) {
        struct linger tmp = {1, 1};
        setsockopt(listenFd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    int flag = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenFd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listenFd, 5);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    // epoll 创建内核时间表
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    
    utils.addfd(epollfd, listenFd, false, listenTrigMode);
    HttpConn::epollfd = epollfd;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    utils.setnonblocking(pipefd[1]);
    utils.addfd(epollfd, pipefd[0], false, 0);

    utils.addSig(SIGPIPE, SIG_IGN);
    utils.addSig(SIGALRM, utils.sigHandle, false);
    utils.addSig(SIGTERM, utils.sigHandle, false);

    alarm(TIMESLOT);
    
    // 工具类，信号和描述符基础操作
    Utils::pipefd = pipefd;
    Utils::epollfd = epollfd;

}

void WebServer::timer(int connfd, struct sockaddr_in clientAddress) {
    users[connfd].init(connfd, clientAddress, root, connTrigMode, closeLog, user, passWord, databaseName);

    // 初始化clientData数据
    // 创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    usersTimer[connfd].address = clientAddress;
    usersTimer[connfd].sockfd = connfd;
    UtilTimer* timer = new UtilTimer;
    timer->userData = &usersTimer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    usersTimer[connfd].timer = timer;
    utils.timerLst.addTimer(timer);

}

// 若有数据传输，则将定时器往后延迟3个单位
// 并对新的定时器在链表上的位置进行跳转
void WebServer::adjustTimer(UtilTimer* timer) {
    time_t cur = time(NULL);
    utils.timerLst.adjustTimer(timer);
    utils.timerLst.adjustTimer(timer);

    LOG_INFO("%s", "adjust timer once");
}

void WebServer::dealTimer(UtilTimer* timer, int sockfd) {
    timer->cb_func(&usersTimer[sockfd]);
    if (timer) {
        utils.timerLst.delTimer(timer);
    }

    LOG_INFO("close fd %d", usersTimer[sockfd].sockfd);
}

bool WebServer::dealClinetData() {
    struct sockaddr_in clientAddress;
    socklen_t clientAddrLength = sizeof(clientAddress);
    if (listenTrigMode == 0) {
        int connfd = accept(listenFd, (struct sockaddr*)&clientAddress, &clientAddrLength);
        if (connfd < 0) {
            LOG_ERROR("%s:errno is :%d", "accept error", error);
            return false;
        }
        if (HttpConn::userCount >= MAX_FD) {
            utils.showError(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, clientAddress);
    } else {
        while (1) {
            int connfd = accept(listenFd, (struct sockaddr*)&clientAddress, &clientAddrLength);
            if (connfd < 0) {
                LOG_ERROR("%s:errno is %d", "accept error", errno);
                break;
            }
            if (HttpConn::userCount >= MAX_FD) {
                utils.showError(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, clientAddress);
        }
        return false;
    }
    return true;
}

bool WebServer::dealWithSignal(bool &timeout, bool& stopServer) {
    int ret = 0;
    char signals[1024];
    ret = recv(pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1) {
        return false;
    } else if (ret == 0) {
        return false;
    } else {
        for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
                case SIGALRM: {
                    timeout = true;
                    break;
                }
                case SIGTERM: {
                    stopServer = true;
                    break;
                }
            }
        }
    }
    return true;
}

void WebServer::dealWithRead(int sockfd) {
    UtilTimer *timer = usersTimer[sockfd].timer;
    // reactor
    if (actorModel == 1) {
        if (timer) {
            adjustTimer(timer);
        }
        // 若监测到读事件，将该事件放入请求队列
        pool->append(users + sockfd, 0);
        while (true) {
            if (users[sockfd].improve == 1) {
                if (users[sockfd].timerFlag == 1) {
                    dealTimer(timer, sockfd);
                    users[sockfd].timerFlag = 0;
                }
                users[sockfd].improve = 0;
                break;
            }
        }
    } else {
        // proactor
        if (users[sockfd].readOnce()) {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].getAddress()->sin_addr));
            // 若监测到读事件，将该事件放入请求队列
            pool->appendP(users + sockfd);
            if (timer) {
                adjustTimer(timer);
            }
        } else {
            dealTimer(timer, sockfd);
        }
    }
}

void WebServer::dealWithWrite(int sockfd) {
    UtilTimer* timer = usersTimer[sockfd].timer;
    // rector
    if (actorModel == 1) {
        if (timer) {
            adjustTimer(timer);
        }
        pool->append(users + sockfd, 1);
        while (true) {
            if (users[sockfd].improve) {
                if (users[sockfd].timerFlag) {
                    dealTimer(timer, sockfd);
                    users[sockfd].timerFlag = 0;
                }
                users[sockfd].improve = 0;
                break;
            }
        }
    } else {
        // proactor
        if (users[sockfd].write()) {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].getAddress()->sin_addr));
            if (timer) {
                adjustTimer(timer);
            }
        } else {
            dealTimer(timer, sockfd);
        }
    }
}

void WebServer::eventLoop() {
    bool timeout = false;
    bool stopServer = false;

    while (!stopServer) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;

            // 处理新到的客户连接
            if (sockfd == listenFd) {
                bool flag = dealClinetData();
                if (flag == false)
                    continue;
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 服务器关闭连接，移除对应的定时器
                UtilTimer* timer = usersTimer[sockfd].timer;
                dealTimer(timer, sockfd);
            } else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
                // 处理信号
                bool flag = dealWithSignal(timeout, stopServer);
                if (flag == false) {
                    LOG_ERROR("%s", "deal client data failure");// 日志！！
                }
            } else if (events[i].events & EPOLLIN) {
                dealWithRead(sockfd);
            } else if (events[i].events & EPOLLOUT) {
                dealWithWrite(sockfd);
            }
        }
        if (timeout) {
            // 超时处理
            utils.timerHandler();
            LOG_INFO("%s", "timer tick");

            timeout = false;
        }
    }

}