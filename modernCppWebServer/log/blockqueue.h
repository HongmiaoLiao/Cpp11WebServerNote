//
// Created by lhm on 2023/3/2.
//

#ifndef MODERNCPPWEBSERVER_BLOCKQUEUE_H
#define MODERNCPPWEBSERVER_BLOCKQUEUE_H

#include <condition_variable>
#include <deque>
#include <mutex>
#include <sys/time.h>
#include <cassert>

template<typename T>
class BlockDeque {
 public:
  explicit BlockDeque(size_t MaxCapacity = 1000);

  ~BlockDeque();

  void Clear();

  bool IsEmpty();

  bool IsFull();

  void Close();

  size_t Size();

  size_t Capacity();

  T Front();

  T Back();

  void PushBack(const T &item);

  void PushFront(const T &item);

  bool Pop(T &item);

  bool Pop(T &item, int timeout);

  void Flush();

 private:
  std::deque<T> deq_;

  size_t capacity_;

  std::mutex mtx_;

  bool is_close_;

  std::condition_variable cond_consumer_;

  std::condition_variable cond_producer_;
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

template<typename T>
void BlockDeque<T>::Close() {
  {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
    is_close_ = true;
  }
  cond_producer_.notify_all();
  cond_consumer_.notify_all();
}

template<typename T>
void BlockDeque<T>::Flush() {
  cond_consumer_.notify_one();
}

template<typename T>
void BlockDeque<T>::Clear() {
  std::lock_guard<std::mutex> locker(mtx_);
  deq_.clear();
}

template<typename T>
T BlockDeque<T>::Front() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.front();
}

template<typename T>
T BlockDeque<T>::Back() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.back();
}

template<typename T>
size_t BlockDeque<T>::Size() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size();
}

template<typename T>
size_t BlockDeque<T>::Capacity() {
  std::lock_guard<std::mutex> locker(mtx_);
  return capacity_;
}

template<typename T>
void BlockDeque<T>::PushBack(const T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_) {
    cond_producer_.wait(locker);
  }
  deq_.push_back(item);
  cond_consumer_.notify_one();
}

template<typename T>
void BlockDeque<T>::PushFront(const T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_) {
    cond_producer_.wait(locker);
  }
  deq_.push_front(item);
  cond_consumer_.notify_one();
}

template<typename T>
bool BlockDeque<T>::IsEmpty() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.empty();
}

template<typename T>
bool BlockDeque<T>::IsFull() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size() >= capacity_;
}

template<typename T>
bool BlockDeque<T>::Pop(T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    cond_consumer_.wait(locker);
    if (is_close_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  cond_producer_.notify_one();
  return true;
}

template<typename T>
bool BlockDeque<T>::Pop(T &item, int timeout) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    if (cond_consumer_.wait_for(locker, std::chrono::seconds(timeout))
        == std::cv_status::timeout) {
      return false;
    }
    if (is_close_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  cond_producer_.notify_one();
  return true;
}

#endif // MODERNCPPWEBSERVER_BLOCKQUEUE_H
