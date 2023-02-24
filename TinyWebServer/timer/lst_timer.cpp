#include "lst_timer.h"
#include "../http/http_conn.h"

void cb_func(clientData* userData) {
    // 删除非活动连接在socket上的注册事件
    epoll_ctl(Utils::epollfd, EPOLL_CTL_DEL, userData->sockfd, 0);
    assert(userData);
    // 关闭文件描述符
    close(userData->sockfd);
    // 减少连接数
    HttpConn::userCount--;
}

SortTimerLst::SortTimerLst() {
    head = NULL;
    tail = NULL;
}

SortTimerLst::~SortTimerLst() {
    UtilTimer* tmp = head;
    while (tmp) {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

// 添加定时器，内部调用私有成员add_timer
void SortTimerLst::addTimer(UtilTimer* timer) {
    if (!timer) {
        return;
    }
    if (!head) {
        head = tail = timer;
        return;
    }
    // 如果新的定时器超时时间小于当前头部节点
    // 直接将当前定时器节点作为头部节点
    if (timer->expire < head->expire) {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    // 否则调用私用成员，调整内部节点
    addTimer(timer, head);
}

// 私有成员，被公有成员addTimer和adjustTimer调用
// 主要用于调整链表内部节点
void SortTimerLst::addTimer(UtilTimer* timer, UtilTimer* lstHead) {
    UtilTimer* prev = lstHead;
    UtilTimer* tmp = prev->next;
    // 遍历当前节点之后的链表，按照超时时间找到目标定时器对应的位置，常规双向链表插入操作
    while (tmp) {
        if (timer->expire < tmp->expire) {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    // 遍历完发现，目标定时器需要放到尾节点处
    if (!tmp) {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

// 调整定时器，任务发生变化时，调整定时器在链表中的位置
void SortTimerLst::adjustTimer(UtilTimer* timer) {
    if (!timer) {
        return;
    }
    UtilTimer* tmp = timer->next;
    // 被调整的定时器在链表尾部
    // 定时器超时值仍然小于下一个定时器超时值，不调整
    if (!tmp || (timer->expire < tmp->expire)) {
        return;
    }
    // 被调整定时器是链表头结点，将定时器取出，重新插入
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        addTimer(timer, head);
    } else {
        //被调整定时器在内部，将定时器取出，重新插入
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        addTimer(timer, timer->next);
    }
}

void SortTimerLst::delTimer(UtilTimer* timer) {
    if (!timer) {
        return;
    }
    if ((timer == head) && (timer == tail)) {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail) {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

// 定时任务处理函数
void SortTimerLst::tick() {
    if (!head) {
        return;
    }
    // 获取当前时间
    time_t cur = time(NULL);
    UtilTimer* tmp = head;
    // 遍历定时器链表
    while (tmp) {
        // 链表容器为升序排列
        // 当前时间小于定时器的超过时间，后面的定时器也没有到期
        if (cur < tmp->expire) {
            break;
        }
        // 当前定时器到期，则调用回调函数，执行定时事件
        tmp->cb_func(tmp->userData);
        // 将处理后的定时器从来链表容器中删除，并重置头结点
        head = tmp->next;
        if (head) {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void Utils::init(int timeslot) {
    timeSlot = timeslot;
}

// 对文件描述符设置非阻塞
int Utils::setnonblocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

// 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int trigMode) {
    epoll_event event;
    event.data.fd = fd;
    if (trigMode == 1) {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    } else {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
    
}

// 信号处理函数
void Utils::sigHandle(int sig) {
    // 为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

// 设置信号函数
void Utils::addSig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

// 定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timerHandler() {
    timerLst.tick();
    alarm(timeSlot);
}

void Utils::showError(int connfd, const char* info) {
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::pipefd = 0;
int Utils::epollfd = 0;
