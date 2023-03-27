// #include "./pool/threadpool.h"
// #include <ctime>
// #include <iostream>
// #include "./test/test.h"

#include <unistd.h>
#include "server/webserver.h"

int main() {
  // tmp_test::TestLog();
  // tmp_test::TestThreadPool();
  // 需要在centos上输入mysql_config --cflags --libs查找三个链接库路径，如下
  // 并在编译选项添加-L/usr/lib64/mysql -lmysqlclient -I/usr/include/mysql
  // 将GCC升级到4.9以上，支持正则
  // 注意路径名过长，导致response判断路径的stat错误
  WebServer server(
      1316, 3, 60000, false,  /* 端口 ET模式 timeout_ms 优雅退出  */
      3306, "jiyu", "L248132240", "tinywebserver",  /* Mysql配置 */
      12, 6, true, 1, 1024);  /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */

  server.Start();
  return 0;
}
