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

  void Adjust(int id, int new_expires);

  void Add(int id, int time_out, const TimeoutCallBack &cb);

  void DoWork(int id);

  void Clear();

  void Tick();

  void Pop();

  int GetNextTick();
 private:
  void Del_(size_t i);

  void SiftUp_(size_t i);

  bool SiftDown_(size_t index, size_t n);

  void SwapNode_(size_t i, size_t j);

  std::vector<TimerNode> heap_;

  std::unordered_map<int, size_t> ref_;

};

#endif //MODERNCPPWEBSERVER_TIMER_HEAPTIMER_H_
