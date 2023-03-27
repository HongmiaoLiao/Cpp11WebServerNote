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

typedef std::function<void()> TimeoutCallBack; 一个仿函数类型，用于超时事件的执行

typedef std::chrono::high_resolution_clock Clock; 时钟类型

typedef std::chrono::milliseconds MS; 毫秒类型

typedef Clock::time_point TimeStamp; 时间戳类型（19700101开始）

TimerNode：一个计时器节点的结构，内含计时器id，超时时间戳，执行超时事件的仿函数，并重载了小于号运算符，使用超时时间作为比较

HeapTimer类型：基于堆（超时时间早的在堆顶）的定时器类型。实际使用中，没有使用DoWork和Del_删除堆中间的节点，所以应该可以用priority_queue容器来封装TimerNode会比较方便

构造函数：将堆的初始大小置为64

析构函数：调用Clear清空映射字典和堆容器

成员函数开头都会先断言执行操作的下标是否在堆容器的范围内

私有成员变量：

* heap_: 存放计时器节点的容器，作为堆的基本容器
* ref_：用于记录计时器节点id和堆内位置的映射关系的字典

私有成员函数：

Del_：删除指定位置节点

* 将要删除的节点和堆尾元素交换，然后以从堆尾换过来的节点为中心进行调整
* 先向下调整，如果没有进行向下调整，则进行向上调整
* 最后从映射字典中删除记录，从堆中弹出最后一个节点元素

SiftUp_: 将在堆下层的节点i，上移至对应位置，直到它父节点小于它或到达根节点

* 如果父节点j的值大于i的值，则向上交换，使值小的在上方，直到根节点（第0）位置

SiftDown_：将堆上层的节点i，下移至对应位置，直到它的两个子结点都比它要大或者到达了末尾n

* 选择左右（如果有右）两边的比较小的子结点，如果子结点比父节点小，父节点才向下交换
* 返回值表示是否进行了下移，给添加和删除节点的成员函数使用

SwapNode_：交换堆中两个节点的位置（不能任意交换，会破坏堆结构，只能在需要时交换）

* 交换在堆中的位置，并且需要在id和位置的映射字典中更新

公有成员函数：

Adjust：调整指定id的节点，将其超时时间设定为timeout之后

* 根据id从映射中取出在堆中的位置，重设时间戳为当前时间加timeout后，更具新的时间调整堆

Add：向堆中添加一个结点，并调整堆，如果已有，则直接调整堆

* 堆尾插入，只需向上调整
* 堆内修改，需要向上或向下调整

DoWork：删除指定id节点，并触发回调函数

* 先调用仿函数对象（调用重载的()），再删除节点

Clear：清空映射字典和堆容器

Tick：清除超时节点，即不断从堆顶取出定时器节点，调用回调函数，直到堆顶节点不超时或堆为空

Pop：弹出堆顶元素，即删除第0个元素

GetNextTick：清除超时节点，取出还未超时的最近节点的剩余时间

* 先调用Tick把超时的回调函数都执行了，超时节点都删除了，如果堆不为空，则取出堆顶元素的超时时间和当前时间的差值
