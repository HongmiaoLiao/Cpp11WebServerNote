# server/webserver.h、server/webserver.cpp

## 使用库

* unistd 头文件，定义杂项符号常量和类型，并声明杂项函数。除所示外，未指定常量的实际值。

```C++
#include <unistd.h>

// getcwd 函数，将当前工作目录的绝对路径名复制到 buf 指向的数组中，size为该数组的大小。返回以null结尾的绝对路径字符串
char *getcwd(char *buf, size_t size);

```

* string.h 头文件

```C++
#include <string.h>

// strncat函数,将不超过 n个字（NULL 字符及其后面的字节不追加）从s2指向的数组附加到s1指向的字符串的末尾。s2的初始字节将覆盖 s1 末尾的NULL字符。 终止 NUL 字符始终追加到结果中。如果在重叠的对象之间进行复制，则行为是未定义的。
char *strncat(char *restrict s1, const char *restrict s2, size_t n);


```

## 笔记
