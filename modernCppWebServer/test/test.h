//
// Created by lhm on 2023/3/4.
//

#ifndef MODERNCPPWEBSERVER_TEST_TEST_H_
#define MODERNCPPWEBSERVER_TEST_TEST_H_

#include "../log/log.h"
#include "../pool/threadpool.h"
#include <features.h>

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

namespace tmp_test {

void TestLog() {
  int cnt = 0;
  int level = 0;
  Log::Instance()->init(level, "./testlog1", ".log", 0);
  for (level = 3; level >= 0; --level) {
    Log::Instance()->SetLevel(level);
    for (int j = 0; j < 10000; ++j) {
      for (int i = 0; i < 4; ++i) {
        LOG_BASE(i, "%s 1111111111 %d ============= ", "Test", ++cnt);
      }
    }
  }
  cnt = 0;
  Log::Instance()->init(level, "./testlog2", ".log", 0);
  for (level = 0; level < 4; ++level) {
    Log::Instance()->SetLevel(level);
    for (int j = 0; j < 10000; ++j) {
      for (int i = 0; i < 4; ++i) {
        LOG_BASE(i, "%s 2222222222 %d ============= ", "Test", ++cnt);
      }
    }
  }
}

void ThreadLogTask(int i, int cnt) {
  for (int j = 0; j < 10000; ++j) {
    LOG_BASE(i, "PID: [%04d] ======= %05d ======= ", gettid(), ++cnt);
  }
}

void TestThreadPool() {
  Log::Instance()->init(0, "./TestThreadPool", ".log", 5000);
  ThreadPool thread_pool(6);
  for (int i = 0; i < 18; ++i) {
    thread_pool.AddTask(std::bind(ThreadLogTask, i % 4, i * 10000));
  }
  getchar();
}

}

#endif //MODERNCPPWEBSERVER_TEST_TEST_H_
