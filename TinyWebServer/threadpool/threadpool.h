#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template<typename T>
class ThreadPool {
public:
    // threadNumber是线程池中线程的数量，maxRequests是请求队列中最多允许的、等待处理的请求的数量
    ThreadPool(int actorModel, ConnectionPool* connPool, int threadNumber=8, int maxRequests=10000);
    ~ThreadPool();
    bool append(T* request, int state); // 向请求队列中插入任务请求
    bool appendP(T* request);   // 应该是弃用的版本

private:
    // 工作线程运行的函数，它不断从工作队列中取出任务并执行
    static void* worker(void* arg);
    void run();

    int threadNumber;   // 线程池中的线程数
    int maxRequests;    // 请求队列中允许的最大请求数
    pthread_t* threads;  // 描述线程池的数组，其大小为threadNumber
    std::list<T*> workQueue;    // 请求队列
    Locker queueLocker; // 保护请求队列的互斥锁
    Sem queueStat;  // 是否有任务需要处理
    ConnectionPool* connPool;   // 数据库连接池指针
    int actorModel; // 模型切换
};


template <typename T>
ThreadPool<T>::ThreadPool(int actorModel, ConnectionPool* connPool, int threadNumber, int maxRequests)
: actorModel(actorModel), threadNumber(threadNumber), maxRequests(maxRequests), threads(NULL), connPool(connPool) {
    if (threadNumber <= 0 || maxRequests <= 0) {
        throw std::exception();
    }
    this->threads = new pthread_t[threadNumber];
    if (!threads) {
        throw std::exception();
    }
    for (int i = 0; i < threadNumber; ++i) {

        // 循环创建线程，并将工作线程按要求进行运行
        if (pthread_create(threads+i, NULL, worker, this) != 0) {
            delete[] threads;
            throw std::exception();
        }
        // 将线程进行分离后，不用单独对工作线程进行回收
        if (pthread_detach(threads[i])) {
            delete[] threads;
            throw std::exception();
        }
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool() {
    delete[] threads;
}

template <typename T>
bool ThreadPool<T>::append(T* request, int state) {
    queueLocker.lock();
    // 根据硬件，预先设置请求队列的最大值
    if (workQueue.size() >= maxRequests) {
        queueLocker.unlock();
        return false;
    }
    request->state = state;
    // 添加任务
    workQueue.push_back(request);
    queueLocker.unlock();
    // 信号量提醒有任务要处理
    queueStat.post();
    return true;
}

template <typename T>
bool ThreadPool<T>::appendP(T *request){
    queueLocker.lock();
    if (workQueue.size() >= maxRequests)
    {
        queueLocker.unlock();
        return false;
    }
    workQueue.push_back(request);
    queueLocker.unlock();
    queueStat.post();
    return true;
}

template<typename T>
void* ThreadPool<T>::worker(void* arg) {
    // 将参数强转为线程池类，调用成员方法
    ThreadPool* pool = (ThreadPool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void ThreadPool<T>::run() {
    while (true) {
        queueStat.wait();
        queueLocker.lock();
        if (workQueue.empty()) {
            queueLocker.unlock();
            continue;
        }
        // 从请求队列中取出第一个任务，将任务从请求队列删除
        T* request = workQueue.front();
        workQueue.pop_front();
        queueLocker.unlock();
        if (!request) {
            continue;
        }
        if (actorModel == 1) {
            if (request->state == 0) {
                if (request->readOnce()) {
                    request->improve = 1;
                    ConnectionRAII mysqlConn(&request->mysql, connPool);
                    request->process();
                } else {
                    request->improve = 1;
                    request->timerFlag = 1;
                }
            } else {
                if (request->write()) {
                    request->improve = 1;
                } else {
                    request->improve = 1;
                    request->timerFlag = 1;
                }
            }
        } else {
            ConnectionRAII mysqlConn(&request->mysql, connPool);
            request->process();
        }
    }
}

#endif