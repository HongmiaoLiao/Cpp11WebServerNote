//
// Created by lhm on 2023/3/21.
//

#ifndef MODERNCPPWEBSERVER_TIMER_HEAPTIMER_H_
#define MODERNCPPWEBSERVER_TIMER_HEAPTIMER_H_

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>

#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
  int id;
  TimeStamp expires;
  TimeoutCallBack cb;
  bool operator<(const TimerNode &t) const {
    return expires < t.expires;
  }
};

class HeapTimer {
 public:
  HeapTimer() {
    heap_.reserve(64);
  }

  ~HeapTimer() {
    Clear();
  }

  // 调整指定id的节点，将其超时时间设定为timeout之后
  void Adjust(int id, int new_expires);

  // 向堆中添加一个结点，并调整堆，如果已有，则直接调整堆
  void Add(int id, int time_out, const TimeoutCallBack &cb);

  // 删除指定id节点，并触发回调函数
  void DoWork(int id);

  // 清空映射字典和堆容器
  void Clear();

  // 清除超时节点，即不断从堆顶取出定时器节点，调用回调函数，直到堆顶节点不超时或堆为空
  void Tick();

  // 弹出堆顶元素，即删除第0个元素
  void Pop();

  // 清除超时节点，取出还未超时的最近节点的剩余时间
  int GetNextTick();
 private:
  // 删除指定位置节点
  void Del_(size_t i);

  // 将在堆下层的节点i，上移至对应位置，直到它父节点小于它或到达根节点
  void SiftUp_(size_t i);

  // 将堆上层的节点i，下移至对应位置，直到它的两个子结点都比它要大或者到达了末尾n
  bool SiftDown_(size_t index, size_t n);

  // 交换堆中两个节点的位置（不能任意交换，会破坏堆结构，只能在需要时交换）
  void SwapNode_(size_t i, size_t j);

  std::vector<TimerNode> heap_; // 存放计时器节点的容器，作为堆的基本容器

  std::unordered_map<int, size_t> ref_; // 用于记录计时器节点id和堆内位置的映射关系的字典

};

#endif //MODERNCPPWEBSERVER_TIMER_HEAPTIMER_H_
