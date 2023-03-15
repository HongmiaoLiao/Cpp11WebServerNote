//
// Created by lhm on 2023/3/2.
//

#ifndef MODERNCPPWEBSERVER_BLOCKQUEUE_H
#define MODERNCPPWEBSERVER_BLOCKQUEUE_H

#include <condition_variable>
#include <deque>
#include <mutex>
#include <sys/time.h> // 这里似乎没用上
#include <cassert>
#include <chrono>

template<typename T>
class BlockDeque {
 public:
  explicit BlockDeque(size_t MaxCapacity = 1000);

  ~BlockDeque();

  // 清空所有队列资源
  void Clear();

  bool IsEmpty();

  bool IsFull();

  // 关闭阻塞队列，释放所用资源
  void Close();

  // 返回队列大小
  size_t Size();

  // 返回阻塞队列容量
  size_t Capacity();

  // 返回队列最前方元素
  T Front();

  // 返回队列最后方元素
  T Back();

  // 将一个item放到双端队列尾部
  void PushBack(const T &item);

  // 将一个item放到双端队列头部
  void PushFront(const T &item);

  // 当双端队列非空时，从双端队列头部弹出一个元素，否则阻塞
  bool Pop(T &item);

  // 当双端队列非空时，从双端队列头部弹出一个元素，否则阻塞timeout秒
  bool Pop(T &item, int timeout);

  // 释放一个消费者条件变量
  void Flush();

 private:
  std::deque<T> deq_; // 双端队列，存放具体内容

  size_t capacity_; // 阻塞队列的最大容量

  std::mutex mtx_;  // 互斥锁，用于队列资源的互斥访问

  bool is_close_; // 标识阻塞队列是否关闭

  std::condition_variable cond_consumer_; // 用于消费者的条件变量，用于在队列为空的时候阻塞，不为空时访问

  std::condition_variable cond_producer_; // 用于生产者的条件变量，用于在队列满时阻塞，不满时访问
};

template<typename T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity)
    : capacity_(MaxCapacity), is_close_(false) {
  assert(MaxCapacity > 0);
}

template<typename T>
BlockDeque<T>::~BlockDeque<T>() {
  Close();
}

// 关闭阻塞队列，释放所用资源
template<typename T>
void BlockDeque<T>::Close() {
  {
    // 这里的大括号是为了在锁退出作用域时释放，而不是函数结束再释放，提高性能
    std::lock_guard<std::mutex> locker(mtx_);
    // 清空队列内容
    deq_.clear();
    // 标识关闭
    is_close_ = true;
  }
  // 释放条件变量
  cond_producer_.notify_all();
  cond_consumer_.notify_all();
}

// 释放一个消费者条件变量
// 这里可能存在问题，因为不能保证消费完队列所有的内容
template<typename T>
void BlockDeque<T>::Flush() {
  cond_consumer_.notify_one();
}

// 清空所有队列资源
template<typename T>
void BlockDeque<T>::Clear() {
  std::lock_guard<std::mutex> locker(mtx_);
  deq_.clear();
}

// 返回队列最前方元素
template<typename T>
T BlockDeque<T>::Front() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.front();
}

// 返回队列最后方元素
template<typename T>
T BlockDeque<T>::Back() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.back();
}

// 返回队列大小
template<typename T>
size_t BlockDeque<T>::Size() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size();
}

// 返回阻塞队列容量
template<typename T>
size_t BlockDeque<T>::Capacity() {
  std::lock_guard<std::mutex> locker(mtx_); // capacity_初始化后就不会改变，应该不用上锁
  return capacity_;
}

// 将一个item放到双端队列尾部
template<typename T>
void BlockDeque<T>::PushBack(const T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_) {
    // wait(locker)先阻塞这个线程，然后释放锁locker，直到收到notice后从此处运行
    // 这里用while而不是if的原因是：在这个线程阻塞时，其他线程可能也会吧deq_填满。
    cond_producer_.wait(locker);
  }
  // 添加到队列中，并释放一个消费者线程
  deq_.push_back(item);
  cond_consumer_.notify_one();
}

// 将一个item放到双端队列头部
template<typename T>
void BlockDeque<T>::PushFront(const T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_) {
    // wait(locker)先阻塞这个线程，然后释放锁locker，直到收到notice后从此处运行
    // 这里用while而不是if的原因是：在这个线程阻塞时，其他线程可能也会吧deq_填满。
    cond_producer_.wait(locker);
  }
  // 添加到队列中，并释放一个消费者线程
  deq_.push_front(item);
  cond_consumer_.notify_one();
}

// 判断双端队列是否为空
template<typename T>
bool BlockDeque<T>::IsEmpty() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.empty();
}

// 判断双端队列释放已满
template<typename T>
bool BlockDeque<T>::IsFull() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size() >= capacity_;
}

// 当双端队列非空时，从双端队列头部弹出一个元素至参数item，否则阻塞，失败返回false
template<typename T>
bool BlockDeque<T>::Pop(T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    // wait(locker)先阻塞这个线程，然后释放锁locker，直到收到notice后从此处运行
    // 这里用while而不是if的原因是：在这个线程阻塞时，其他线程可能也会吧deq_填满。
    // 这里先判断是否关闭
    if (is_close_) {
      return false;
    }
    cond_consumer_.wait(locker);
  }
  // 首个元素复制给item
  item = deq_.front();
  // 队列弹出首个元素，并唤一个生产者线程
  deq_.pop_front();
  cond_producer_.notify_one();
  return true;
}

// 当双端队列非空时，从双端队列头部弹出一个元素至参数item，否则阻塞timeout秒，失败返回false
template<typename T>
bool BlockDeque<T>::Pop(T &item, int timeout) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    if (is_close_) {
      return false;
    }
    if (cond_consumer_.wait_for(locker, std::chrono::seconds(timeout))
        == std::cv_status::timeout) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  cond_producer_.notify_one();
  return true;
}

#endif // MODERNCPPWEBSERVER_BLOCKQUEUE_H
