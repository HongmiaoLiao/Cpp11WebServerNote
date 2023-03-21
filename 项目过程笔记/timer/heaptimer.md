# timer/heaptimer.cpp、timer/heaptimer.h

## 使用库

* chrono 头文件

```C++
#include <chrono>

// high_resolution_clock为最短的时钟。它可能是system_clock或steady_clock的同义词。
using high_resolution_clock = steady_clock;
class steady_clock; // steady_clock专门用于计算时间间隔。

// time_point 类型，time_point对象表示相对于epoch的时间点。在内部，该对象存储一个持续时间类型的对象，并使用Clock类型作为其epoch的引用。
template <class Clock, class Duration = typename Clock::duration>  class time_point;

// milliseconds 类型，它是duration实例化的类型定义，具有以下成员类型:rep，period
typedef duration < /* see rep below */, milli > milliseconds;

// now 函数，返回high_resolution_clock框架中的当前时间点。
std::chrono::high_resolution_clock::now
static time_point now() noexcept;

// duration_cast 函数，将dtn的值转换为其他持续时间类型，同时考虑它们的周期差异。
template <class ToDuration, class Rep, class Period>  constexpr ToDuration duration_cast (const duration<Rep,Period>& dtn);

// std::chrono::duration::count 函数
constexpr rep count() const;
返回duration对象的内部计数(即表示值)。
```

## 笔记
