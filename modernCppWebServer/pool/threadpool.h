//
// Created by lhm on 2023/2/22.
//

#ifndef MODERNCPPWEBSERVER_THREADPOOL_H
#define MODERNCPPWEBSERVER_THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>
#include <memory>
#include <utility>

class ThreadPool {
public:
    // 线程池构造函数使用关键字explicit阻止构造函数在隐式转换中使用
    // 初始化列表调用make_shared函数，创建线程池结构体，并给智能指针赋值
    explicit ThreadPool(size_t thread_count = 8) : pool_(std::make_shared<Pool>()) {
        // 确保线程池线程总量要大于0
        assert(thread_count > 0);
        // 使用一个循环逐个初始化所有线程
        for (size_t i = 0; i < thread_count; ++i) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> locker(pool->mtx); // 这里上锁
                while (true) {
                    if (!pool->tasks.empty()) {
                        // 任务队列，非空，则从队列中取出任务，取出的任务对象（仿函数）采用移动赋值，更高效
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();    // 从队列中取出任务后解锁
                        task(); // 执行任务
                        locker.lock();  // 重新上锁，互斥访问队列
                    } else if (pool->is_closed) {
                        // 线程池关闭，退出循环，即退出线程
                        break;
                    } else {
                        // 线程池为空，阻塞一个线程池中的线程
                        pool->cond.wait(locker);
                    }
                }
            }).detach();    // 拆离相关联的线程。操作系统负责释放终止的线程资源。
        }
    }

    // 无参构造函数，使用=default关键字，令编译器自动生成（没有创建线程和任务队列）
    ThreadPool() = default;

    // 移动构造函数，使用=default关键字，令编译器自动生成
    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool() {
        if (static_cast<bool>(pool_)) {
            {
                // 这里有大括号，是为了提前是否锁，locker在作用域结束后释放锁，而不是在析构函数结束再释放
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->is_closed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<typename F>
    void AddTask(F&& task) {
        {
            // 这里有大括号，是为了提前是否锁，locker在作用域结束后释放锁，而不是在析构函数结束再释放
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx; // 互斥锁
        std::condition_variable cond;   // 条件变量
        bool is_closed = false; // 标识线程池是否关闭
        std::queue<std::function<void()>> tasks;    // 任务线程的队列
    };

    std::shared_ptr<Pool> pool_;    // 任务队列的共享资源智能指针
};

#endif //MODERNCPPWEBSERVER_THREADPOOL_H
