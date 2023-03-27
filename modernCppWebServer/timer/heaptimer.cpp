//
// Created by lhm on 2023/3/21.
//

#include "heaptimer.h"

// 将在堆下层的节点i，上移至对应位置，直到它父节点小于它或到达根节点
void HeapTimer::SiftUp_(size_t i) {
  assert(i >= 0 && i < heap_.size());
  size_t j = (i - 1) / 2; // j为父节点位置
  while (j >= 0) {
    // 如果父节点j的值大于i的值，则向上交换，使值小的在上方，直到它父节点小于它或到根节点（第0）位置
    // 原代码没有限制i == 0会直接进入死循环，这里i == 0后，直接break掉
    if (heap_[j] < heap_[i] || i == 0) {
      break;
    }
    SwapNode_(i, j);
    i = j;
    j = (i - 1) / 2;
  }
}

// 交换堆中两个节点的位置（不能任意交换，会破坏堆结构，只能在需要时交换）
void HeapTimer::SwapNode_(size_t i, size_t j) {
  assert(i >= 0 && i <= heap_.size());
  assert(j >= 0 && j <= heap_.size());
  // 交换在堆中的位置，并且需要在id和位置的映射字典中更新
  std::swap(heap_[i], heap_[j]);
  ref_[heap_[i].id] = i;
  ref_[heap_[j].id] = j;
}

// 将堆上层的节点i，下移至对应位置，直到它的两个子结点都比它要大或者到达了末尾n，返回值表示是否进行了下移
bool HeapTimer::SiftDown_(size_t index, size_t n) {
  // 我觉得可以省略n，直接去heap_的大小（反正对堆的修改也没加锁，没有多线程执行）
  assert(index >= 0 && index < heap_.size());
  assert(n >= 0 && n <= heap_.size());
  size_t i = index;
  size_t j = i * 2 + 1; // j是子节点
  while (j < n) {
    if (j + 1 < n && heap_[j + 1] < heap_[j]) {
      // 选择左右（如果有右）两边的比较小的子结点
      ++j;
    }
    if (heap_[i] < heap_[j]) {
      break;
    }
    // 如果子结点比父节点小，父节点才向下交换
    SwapNode_(i, j);
    i = j;
    j = i * 2 + 1;
  }
  // 返回值表示是否进行了下移，给添加和删除节点的成员函数使用
  return i > index;
}

// 向堆中添加一个结点，并调整堆，如果已有，则直接调整堆
void HeapTimer::Add(int id, int time_out, const TimeoutCallBack &cb) {
  assert(id >= 0);
  size_t i;
  if (ref_.count(id) == 0) {
    /* 原作者注释 新结点：堆尾插入，调整堆 */
    i = heap_.size();
    ref_[id] = i;
    // Initialization list 调用构造函数
    heap_.push_back({id, Clock::now() + MS(time_out), cb});
    SiftUp_(i); // 堆尾插入，只需向上调整
  } else {
    /* 已有结点：调整堆 */
    i = ref_[id];
    heap_[i].expires = Clock::now() + MS(time_out);
    heap_[i].cb = cb;
    // 堆内修改，需要向上或向下调整
    if (!SiftDown_(i, heap_.size())) {
      SiftUp_(i);
    }
  }
}

// 删除指定id节点，并触发回调函数
void HeapTimer::DoWork(int id) {
  /* 原作者注释 删除指定id节点，并触发回调函数 */
  if (heap_.empty() || ref_.count(id) == 0) {
    return ;
  }
  size_t i = ref_[id];
  TimerNode node = heap_[i];
  // 先调用仿函数对象（调用重载的()），再删除
  node.cb();
  Del_(i);
}

// 删除指定位置节点
void HeapTimer::Del_(size_t index) {
  /* 原作者注释 删除指定位置的节点 */
  assert(!heap_.empty() && index >= 0 && index < heap_.size());
  /* 原作者注释 将要删除的结点换到队尾，然后调整堆 */
  size_t i = index;
  size_t n = heap_.size() - 1;
  assert(i <= n);
  // 将要删除的节点和堆尾元素交换，然后以从堆尾换过来的节点为中心进行调整
  if (i < n) {
    SwapNode_(i, n);
    // 先向下调整，如果没有进行向下调整，则进行向上调整
    if (!SiftDown_(i, n)) {
      SiftUp_(i);
    }
  }
  /* 原作者注释 队尾元素删除 */
  // 最后从映射字典中删除记录，从堆中弹出最后一个节点元素
  ref_.erase(heap_.back().id);
  heap_.pop_back();
}

// 调整指定id的节点，将其超时时间设定为timeout之后
void HeapTimer::Adjust(int id, int timeout) {
  /* 调整指定id的结点 */
  assert(!heap_.empty() && ref_.count(id) > 0);
  // 根据id从映射中取出在堆中的位置，重设时间戳为当前时间加timeout后，更具新的时间调整堆
  heap_[ref_[id]].expires = Clock::now() + MS(timeout);
  // SiftDown_(ref_[id], heap_.size()); // 这里不一定只向下调整
  if (!SiftDown_(ref_[id], heap_.size())) {
    SiftUp_(ref_[id]);
  }
}

// 清除超时节点，即不断从堆顶取出定时器节点，调用回调函数，直到堆顶节点不超时或堆为空
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

// 弹出堆顶元素，即删除第0个元素
void HeapTimer::Pop() {
  assert(!heap_.empty());
  Del_(0);
}

// 清空映射字典和堆容器
void HeapTimer::Clear() {
  ref_.clear();
  heap_.clear();
}

// 清除超时节点，取出还未超时的最近节点的剩余时间
int HeapTimer::GetNextTick() {
  Tick();
  size_t res = -1;
  if (!heap_.empty()) {
    // 如果堆不为空，则取出堆顶元素的超时时间和当前时间的差值
    res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
    if (res < 0) {
      res = 0;
    }
  }
  return res;
}















