#include "./pool/threadpool.h"
#include <ctime>
#include <iostream>
#include "./test/test.h"

int main() {
  tmp_test::TestLog();
  tmp_test::TestThreadPool();
  return 0;
}
