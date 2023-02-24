#include "http_conn.h"

#include <mysql/mysql.h>
#include <fstream>

// 定义http响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file form form this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the request file.\n";

Locker lock;
map<string, string> users;
int HttpConn::userCount = 0;
int HttpConn::epollfd = -1;

// 循环读取客户数据，直到无数据可读或对方关闭连接
// 非阻塞ET工作模式下，需要一次性将数据读完
bool HttpConn::readOnce() {
    if (readIdx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytesRead = 0;
    //LT 读取数据
    if (trigMode == 0) {
        // 从套接字接收数据存储在readBuf缓冲区
        bytesRead = recv(sockfd, readBuf+readIdx, READ_BUFFER_SIZE-readIdx, 0);
        readIdx += bytesRead;   // 修改读取字节数
        if (bytesRead <= 0) {
            return false;
        }
        return true;
    } else {
        // ET 读数据
        while (true) {
            bytesRead = recv(sockfd, readBuf+readIdx, READ_BUFFER_SIZE-readIdx, 0);
            if (bytesRead == -1) {
                // 非阻塞ET模式下，需要一次性将数据读完
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            } else if (bytesRead == 0) {
                return false;
            }
            readIdx += bytesRead;
        }
        return true;
    }
}

// 对文件描述符设置非阻塞
int setNonBlocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

// 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT, 针对客户端连接的描述符，listenfd不用开启
void addfd(int epollfd, int fd, bool oneShot, int trigMode) {
    epoll_event event;
    event.data.fd = fd;

    if (trigMode == 1) {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    } else {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if (oneShot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

// 从内核事件表删除描述符
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// 将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev, int trigMode) {
    epoll_event event;
    event.data.fd = fd;

    if (trigMode == 1) {
       event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    } else {
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    }

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 同步线程初始化数据库读取表
void HttpConn::initMysqlResult(ConnectionPool *connPoll) {
    // 先从连接池中取一个连接
    MYSQL* mysql = NULL;
    ConnectionRAII mysqlConn(&mysql, connPoll);

    // 在user表中检索username， password数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username, passwd FROM user")) {
        LOG_ERROR("SELECT error: %s\n", mysql_error(mysql));
    }

    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);
    // 返回结果集中的列数
    int numFields = mysql_num_fields(result);
    // 返回所有字段结构的数组
    MYSQL_FIELD* fields = mysql_fetch_field(result);
    // 从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

// 关闭连接，关闭一个连接，客户总量减一
void HttpConn::closeConn(bool realClose) {
    if (realClose && (sockfd != -1)) {
        printf("close %d\n", sockfd);
        removefd(epollfd, sockfd);
        sockfd = -1;
        --userCount;
    }
}

// 初始化连接，从外部调用初始化套接字地址
void HttpConn::init(int sockfd, const sockaddr_in& addr, char* root, int trigMode,
                    int closeLog, string user, string passwd, string sqlname) {
    this->sockfd = sockfd;
    this->address = addr;
    addfd(epollfd, sockfd, true, this->trigMode);
    ++userCount;

    // 当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或访问文件中的内容完全为空
    this->docRoot = root;
    this->trigMode = trigMode;
    this->closeLog = closeLog;

    strcpy(sqlUser, user.c_str());
    strcpy(sqlPassword, passwd.c_str());
    strcpy(sqlName, sqlname.c_str());

    init();
}

// 初始化新接受的连接
// checkState默认为分析请求行状态
void HttpConn::init() {
    mysql = NULL;
    bytesToSend = 0;
    bytesHaveSend = 0;
    checkState = CHECK_STATE_REQUESTLINE;
    linger = false;
    method = GET;
    url = 0;
    version = 0;
    contentLength = 0;
    host = 0;
    startLine = 0;
    checkedIdx = 0;
    readIdx = 0;
    writeIdx = 0;
    cgi = 0;
    state = 0;
    timerFlag = 0;
    improve = 0;

    memset(readBuf, '\0', READ_BUFFER_SIZE);
    memset(writeBuf, '\0', WRITE_BUFFER_SIZE);
    memset(realFile, '\0', FILENAME_LEN);
}

void HttpConn::process() {
    HTTP_CODE readRet = processRead();
    if (readRet == NO_REQUEST) {
        modfd(epollfd, sockfd, EPOLLIN, trigMode);
        return;
    }
    bool writeRet = processWrite(readRet);
    if (!writeRet) {
        closeConn();
    }
    modfd(epollfd, sockfd, EPOLLOUT, trigMode);
}

HttpConn::HTTP_CODE HttpConn::processRead() {
    // 初始化从状态机状态、http请求解析结果
    LINE_STATUS lineStatus = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;
    // 在GET请求报文中，每一行都是\r\n作为结束，所以对报文进行拆解时，仅用从状态机的状态line_status=parse_line())==LINE_OK语句即可。
    // 但，在POST请求报文中，消息体的末尾没有任何字符，所以不能使用从状态机的状态，这里转而使用主状态机的状态作为循环入口条件
    while ((checkState == CHECK_STATE_CONTENT && lineStatus == LINE_OK) || ((lineStatus = parseLine()) == LINE_OK)) {
        text = getLine();
        // startLine是每一个数据行在readBuf中的起始位置
        // checkIdx表示状态机在readBuf中读取的位置
        startLine = checkedIdx;
        // LOG_INFO("%s", text);
        // 主状态机的三种状态转移逻辑
        switch (checkState) {
            case CHECK_STATE_REQUESTLINE: {
                // 解析请求行
                ret = parseRequestLine(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER: {
                // 解析请求头
                ret = parseHeaders(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else if (ret == GET_REQUEST) // 完整解析GET请求后，跳转到报文响应函数
                    return doRequest();
                break;
            }
            case CHECK_STATE_CONTENT: {
                // 解析消息体
                ret = parseContent(text);
                // 完整解析post请求后，跳转到报文响应函数
                if (ret == GET_REQUEST)
                    return doRequest();
                lineStatus = LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_EROR;
        }
    }
    return NO_REQUEST;
}

// 从状态机，用于分析出一行内容
// 返回值为行的读取状态，有LINE_OK, LINE_BAD, LINE_OPEN 
HttpConn::LINE_STATUS HttpConn::parseLine() {
    char temp;
    for(; checkedIdx < readIdx; ++checkedIdx) {
        // temp为将要分析的字节
        temp = readBuf[checkedIdx];
        // 如果当前是\r字符，则有可能会读取到完整行
        if (temp == '\r') {
            // 下一个字符达到了buffer结尾，则接收不完整，要继续接收
            if ((checkedIdx + 1) == readIdx) {
                return LINE_OPEN;
            } else if (readBuf[checkedIdx + 1] == '\n') { // 下一个字符是\n，将\r\n改为\0\0
                readBuf[checkedIdx++] = '\0';
                readBuf[checkedIdx++] = '\0';
                return LINE_OK;
            } 
            // 如果都不符合，则返回语法错误
            return LINE_BAD;
        } else if (temp == '\n') {
            // 如果当前字符是\n，也有可能读取到完整行
            // 一般是上次读取到\r就到buffer末尾了，没有接收完整，再此接收时会出现这种情况
            if (checkedIdx > 1 && readBuf[checkedIdx-1] == '\r') {
                // 前一个字符是\r,则接收完整
                readBuf[checkedIdx - 1] = '\n';
                readBuf[checkedIdx++] = '\n';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    // 没有找到\r\n, 需要继续接收
    return LINE_OPEN;
}

// 解析http请求行，获得请求方法，目标url及http版本号
HttpConn::HTTP_CODE HttpConn::parseRequestLine(char *text) {
    // 在HTTP报文中，请求行用来说明请求类型，要访问的资源以及所使用的HTTP版本，其中各个部分之间通过\t或空格分隔。
    // 请求行中最先含有空格和\t任一字符的位置并返回
    url = strpbrk(text, " \t");
    
    // 如果没有空格或\t，则报文格式有误
    if (!url) {
        return BAD_REQUEST;
    }

    // 将该位置改为\0, 用于将前面数据去除
    *url++ = '\0';
    // 取出数据，并通过与GET和POST比较，以确定请求方式
    char *method = text;
    if (strcasecmp(method, "GET") == 0) {
        this->method = GET;
    } else if (strcasecmp(method, "POST") == 0) {
        this->method = POST;
        cgi = 1;
    } else {
        return BAD_REQUEST;
    }

    // url此时跳过了第一个空格或\t字符，但不知道之后是否还有
    // 将url向后偏移，通过查找，继续跳过空格或\t，指向请求资源的第一个字符
    url += strspn(url, " \t");

    // 使用与判断请求方式相同的逻辑，判断HTTP版本号
    version = strpbrk(url, " \t");
    if (!version) {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    // 仅支持HTTP/1.1
    if (strcasecmp(version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }
    // 对请求资源前7个字符进行判断
    // 这里主要是有些报文的请求资源中会带有http://，这里需要对这种情况单独处理
    if (strncasecmp(url, "http://", 7) == 0) {
        url += 7;
        url = strchr(url, '/');
    }
    //同样增加https情况
    if (strncasecmp(url, "https://", 8) == 0) {
        url += 8;
        url = strchr(url, '/');
    }

    //一般的不会带有上述两种符号，直接是单独的/或/后面带访问资源
    if (!url || url[0] != '/')
        return BAD_REQUEST;
    // 当url为/时，显示判断界面
    if (strlen(url) == 1) {
        strcat(url, "judge.html");
    }

    // 请求行处理完毕，将主状态机转移处理请求头
    checkState = CHECK_STATE_HEADER;
    return NO_REQUEST;
} 

// 解析http请求的一个头部信息
HttpConn::HTTP_CODE HttpConn::parseHeaders(char* text) {
    // 判断是空行还是请求头
    if (text[0] == '\0') {
        // 判断是GET还是POST请求
        if (contentLength != 0) {
            // POST需要跳转到消息体处理状态
            checkState = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        // 解析请求头部连接字段
        text += 11;
        // 跳过空格和\t字符
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            // 如果是长连接，则将linger标志设置为true
            linger = true;
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) {
        // 解析请求头部内容的长度字段
        text += 15;
        text += strspn(text, " \t");
        contentLength = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        // 解析请求头部HOST字段
        text += 5;
        text += strspn(text, " \t");
        host = text;
    } else {
         LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

// 判断http请求是否被完整读入
HttpConn::HTTP_CODE HttpConn::parseContent(char* text) {
    // 判断buffer中是否读取了消息体
    if (readIdx >= (contentLength + checkedIdx)) {
        text[contentLength] = '\0';
        // POST请求中最后输入的用户名和密码
        str = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::doRequest() {
    strcpy(realFile, docRoot);
    int len = strlen(docRoot);
    const char* p = strrchr(url, '/');

    // 处理cgi
    if (cgi == 1 && (*(p+1) == '2' || *(p+1) == '3')) {
        // 判断标志判断是登陆检测还是注册检测
        char flag = url[1];

        char* urlReal = (char *)malloc(sizeof(char) * 200);
        strcpy(urlReal, "/");
        strcat(urlReal, url + 2);
        strncpy(realFile + len, urlReal, FILENAME_LEN - len -1);
        free(urlReal);

        // 将用户名和密码提取出来
        char name[100];
        char password[100];
        int i;
        for (i = 5; str[i] != '&'; ++i) {
            name[i - 5] = str[i];
        }
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; str[i] != '\0'; ++i, ++j) {
            password[j] = str[i];
        }
        password[j] = '\0';

        if (*(p+1) == '3') {
            // 如果是注册，先检测数据库中是否有重名的
            // 没有重名的，进行增加数据
            char* sqlInsert = (char*) malloc(sizeof(char) * 200);
            strcpy(sqlInsert, "INSERT INTO user(username, passwd) values(");
            strcat(sqlInsert, "'");
            strcat(sqlInsert, name);
            strcat(sqlInsert, "', '");
            strcat(sqlInsert, password);
            strcat(sqlInsert, "')");

            if (users.find(name) == users.end()) {
                lock.lock();
                int res = mysql_query(mysql, sqlInsert);
                users.insert(pair<string, string>(name, password));
                lock.unlock();

                if (!res) {
                    strcpy(url, "/log.html");
                } else {
                    strcpy(url, "/registerError.html");
                }
            }
        // 如果是登录，直接判断
        // 若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
        } else if (*(p + 1) == '2') {
            if (users.find(name) != users.end() && users[name] == password) {
                strcpy(url, "/welcome.html");
            } else {
                strcpy(url, "/logError.html");
            }
        }
    }

    if (*(p + 1) == '0') {
        char* urlReal = (char*) malloc(sizeof(char) * 200);
        strcpy(urlReal, "/register.html");
        strncpy(realFile + len, urlReal, strlen(urlReal));

        free(urlReal);
    } else if (*(p + 1) == '1') {
        char* urlReal = (char*) malloc(sizeof(char) * 200);
        strcpy(urlReal, "/log.html");
        strncpy(realFile + len, urlReal, strlen(urlReal));

        free(urlReal);
    } else if (*(p + 1) == '5') {
        char* urlReal = (char*) malloc(sizeof(char) * 200);
        strcpy(urlReal, "/picture.html");
        strncpy(realFile + len, urlReal, strlen(urlReal));

        free(urlReal);
    } else if (*(p + 1) == '6') {
        char* urlReal = (char*) malloc(sizeof(char) * 200);
        strcpy(urlReal, "/video.html");
        strncpy(realFile + len, urlReal, strlen(urlReal));

        free(urlReal);
    } else if (*(p + 1) == '7') {
        char* urlReal = (char*) malloc(sizeof(char) * 200);
        strcpy(urlReal, "/fans.html");
        strncpy(realFile + len, urlReal, strlen(urlReal));

        free(urlReal);
    } else {
        strncpy(realFile + len, url, FILENAME_LEN - len - 1);
    }

    if (stat(realFile, &fileStat) < 0) {
        return NO_REQUEST;
    }
    if (!(fileStat.st_mode & S_IROTH)) {
        return FORBIDDEN_RQEUEST;
    }
    if (S_ISDIR(fileStat.st_mode)) {
        return BAD_REQUEST;
    }

    int fd = open(realFile, O_RDONLY);
    fileAddress = (char*)mmap(0, fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;

}

bool HttpConn::addResponse(const char* format, ...) {
    // 如果写入内容超出writeBuf大小则报错
    if (writeIdx >= WRITE_BUFFER_SIZE) {
        return false;
    }
    // 定义可变参数列表
    va_list arg_list;
    // 将变量arg_list初始化为传入参数
    va_start(arg_list, format);
    // 将数据format从可变参数写入缓冲区写，返回写入数组的长度
    int len = vsnprintf(writeBuf + writeIdx, WRITE_BUFFER_SIZE - 1 - writeIdx, format, arg_list);
    // 如果写入的数据长度超过缓冲区剩余空间，则报错
    if (len >= (WRITE_BUFFER_SIZE - 1 - writeIdx)) {
        va_end(arg_list);
        return false;
    }
    // 更新writeIdx位置
    writeIdx += len;
    // 清空可变参数列表
    va_end(arg_list);

    LOG_INFO("request:%s", writeBuf);

    return true;
}

// 添加状态行
bool HttpConn::addStatusLine(int status, const char* title) {
    return addResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

// 添加表示响应报文的长度
bool HttpConn::addContentLength(int contentLen) {
    return addResponse("Content-Length:%d\r\n", contentLen);
}

// 添加文本类型，这里是html
bool HttpConn::addContentType() {
    return addResponse("Content-Type:%s\r\n", "text/html");
}

// 添加连接状态，通知浏览器端是保持连接还是关闭
bool HttpConn::addLinger() {
    return addResponse("Connection:%s\r\n", (linger == true) ? "keep-alive" : "close");
}

// 添加空行
bool HttpConn::addBlankLine() {
    return addResponse("%s", "\r\n");
}

// 添加消息报头，具体的添加文本长度、连接状态和空行
bool HttpConn::addHeaders(int contentLen) {
    return addContentLength(contentLen) && addLinger() && addBlankLine();
}

// 添加文本content
bool HttpConn::addContent(const char* content) {
    return addResponse("%s", content);
}

bool HttpConn::processWrite(HTTP_CODE ret) {
    switch(ret) {
        case INTERNAL_EROR: {//内部错误 500
            // 状态行
            addStatusLine(500, error_500_title);
            // 消息报头
            addHeaders(strlen(error_500_form));
            if (!addContent(error_500_form)) {
                return false;
            }
            break;
        }
        case BAD_REQUEST: { // 报文语法有误，404
            addStatusLine(404, error_404_title);
            addHeaders(strlen(error_404_form));
            if (!addContent(error_404_form)) {
                return false;
            }
            break;
        }
        case FORBIDDEN_RQEUEST: { // 资源没有访问权限， 403
            addStatusLine(403, error_403_title);
            addHeaders(strlen(error_403_form));
            if (!addContent(error_403_form)) {
                return false;
            }
            break;
        }
        case FILE_REQUEST: { // 文件存在 200
            addStatusLine(200, ok_200_title);
            // 如果请求的资源存在
            if (fileStat.st_size != 0) {
                addHeaders(fileStat.st_size);
                // 第一个iovec指针指向响应报文缓冲区，长度指向writeIdx
                m_iv[0].iov_base = writeBuf;
                m_iv[0].iov_len = writeIdx;
                // 第二个iovec指针指向mmap返回的文件指针，长度指向文件大小
                m_iv[1].iov_base = fileAddress;
                m_iv[1].iov_len = fileStat.st_size;
                ivCount = 2;
                // 发送的全部数据为响应报文头部信息和文件大小
                bytesToSend = writeIdx + fileStat.st_size;
                return true;
            } else {
                // 如果请求的资源大小为0，则返回空白html文件
                const char *okString = "<html><body></body></html>";
                addHeaders(strlen(okString));
                if (!addContent(okString)) {
                    return false;
                }
            }
        }
        default: {
            return false;
        }
    }
    // 出FILE_REQUEST状态外，其余状态只申请一个iovec, 指向响应报文缓冲区
    m_iv[0].iov_base = writeBuf;
    m_iv[0].iov_len = writeIdx;
    ivCount = 1;
    bytesToSend = writeIdx;
    return true;
}

void HttpConn::unmap() {
    if (fileAddress) {
        munmap(fileAddress, fileStat.st_size);
        fileAddress = 0;
    }
}

bool HttpConn::write() {
    int temp = 0;

    // 若要发送的数据长度为0
    // 表示响应报文为空，一般不会出现这种情况
    if (bytesToSend == 0) {
        modfd(epollfd, sockfd, EPOLLIN, trigMode);
        init();
        return true;
    }

    while (1) {
        // 将响应报文的状态行、消息头、空行和响应正文发送给浏览器端
        temp = writev(sockfd, m_iv, ivCount);
        if (temp < 0) {
            if (errno == EAGAIN) {
                modfd(epollfd, sockfd, EPOLLOUT, trigMode);
                return true;
            }
            unmap();
            return false;
        }

        bytesHaveSend += temp;  // 更新已发送字节
        bytesToSend -= temp; 

        // 第一个iovec头部信息的数据已发送完，发送第二个iovec数据
        if (bytesHaveSend >= m_iv[0].iov_len) {
            // 不再继续发送头部信息
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = fileAddress + (bytesHaveSend - writeIdx);
            m_iv[1].iov_len = bytesToSend;
        } else {    // 继续发生第一个iovec头部信息的数据
            m_iv[0].iov_base = writeBuf + bytesHaveSend;
            m_iv[0].iov_len = m_iv[0].iov_len - bytesHaveSend;
        }

        // 判断条件，数据已全部发送完
        if (bytesToSend <= 0) {
            unmap();
            // 在epoll树上重置EPOLLONESHOT事件
            modfd(epollfd, sockfd, EPOLLIN, trigMode);
            // 浏览器的请求为长连接
            if (linger) {
                // 重新初始化HTTP对象
                init();
                return true;
            } else {
                return false;
            }
        }
    }

}

