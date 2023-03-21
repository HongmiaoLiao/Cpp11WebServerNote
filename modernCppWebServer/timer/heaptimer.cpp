//
// Created by lhm on 2023/3/21.
//

#include "heaptimer.h"

void HeapTimer::SiftUp_(size_t i) {
  assert(i >= 0 && i < heap_.size());
  size_t j = (i - 1) / 2;
  while (j >= 0) {
    if (heap_[j] < heap_[i]) {
      break;
    }
    SwapNode_(i, j);
    i = j;
    j = (i - 1) / 2;
  }
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
  assert(i >= 0 && i <= heap_.size());
  assert(j >= 0 && j <= heap_.size());
  std::swap(heap_[i], heap_[j]);
  ref_[heap_[i].id] = i;
  ref_[heap_[j].id] = j;
}

bool HeapTimer::SiftDown_(size_t index, size_t n) {
  assert(index >= 0 && index < heap_.size());
  assert(n >= 0 && n <= heap_.size());
  size_t i = index;
  size_t j = i * 2 + 1;
  while (j < n) {
    if (j + 1 < n && heap_[j + 1] < heap_[j]) {
      ++j;
    }
    if (heap_[i] < heap_[j]) {
      break;
    }
    SwapNode_(i, j);
    i = j;
    j = i * 2 + 1;
  }
  return i > index;
}

void HeapTimer::Add(int id, int time_out, const TimeoutCallBack &cb) {
  assert(id >= 0);
  size_t i;
  if (ref_.count(id) == 0) {
    /* 原作者注释 新结点：堆尾插入，调整堆 */
    i = heap_.size();
    ref_[id] = i;
    heap_.push_back({id, Clock::now() + MS(time_out), cb});
    SiftUp_(i);
  } else {
    /* 已有结点：调整堆 */
    i = ref_[id];
    heap_[i].expires = Clock::now() + MS(time_out);
    heap_[i].cb = cb;
    if (!SiftDown_(i, heap_.size())) {
      SiftUp_(i);
    }
  }
}

void HeapTimer::DoWork(int id) {
  /* 原作者注释 删除指定id节点，并触发回调函数 */
  if (heap_.empty() || ref_.count(id) == 0) {
    return ;
  }
  size_t i = ref_[id];
  TimerNode node = heap_[i];
  node.cb();
  Del_(i);
}

void HeapTimer::Del_(size_t index) {
  /* 原作者注释 删除指定位置的节点 */
  assert(!heap_.empty() && index >= 0 && index < heap_.size());
  /* 原作者注释 将要删除的结点换到队尾，然后调整堆 */
  size_t i = index;
  size_t n = heap_.size() - 1;
  assert(i <= n);
  if (i < n) {
    SwapNode_(i, n);
    if (!SiftDown_(i, n)) {
      SiftUp_(i);
    }
  }
  /* 原作者注释 队尾元素删除 */
  ref_.erase(heap_.back().id);
  heap_.pop_back();
}

void HeapTimer::Adjust(int id, int timeout) {
  /* 调整指定id的结点 */
  assert(!heap_.empty() && ref_.count(id) > 0);
  heap_[ref_[id]].expires = Clock::now() + MS(timeout);
  SiftDown_(ref_[id], heap_.size());
}

void HeapTimer::Tick() {
  /* 清除超时节点 */
  if (heap_.empty()) {
    return;
  }
  while (!heap_.empty()) {
    TimerNode node = heap_.front();
    if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
      break;
    }
    node.cb();
    Pop();
  }
}

void HeapTimer::Pop() {
  assert(!heap_.empty());
  Del_(0);
}

void HeapTimer::Clear() {
  ref_.clear();
  heap_.clear();
}

int HeapTimer::GetNextTick() {
  Tick();
  size_t res = -1;
  if (!heap_.empty()) {
    res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
    if (res < 0) {
      res = 0;
    }
  }
  return res;
}















