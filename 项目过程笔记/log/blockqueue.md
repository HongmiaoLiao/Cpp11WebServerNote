# log/blockqueue.h

## 使用库

mutex、condition_variable在threadpool.md中记录过，重复的内容不再赘述

* deque 头文件，定义双端队列容器模板，双端队列是具有动态大小的序列容器，可以在两端(前面或后面)进行扩展或收缩。常用库不赘述

* mutex 头文件

```C++
#include <mutex>

// lock_guard 类,表示可进行实例化以创建其析构函数解锁 mutex 的对象的模板。
template <class Mutex>
class lock_guard;

std::unique_lock vs std::lock_guard
// lock_guard和unique_lock几乎是一样的东西;lock_guard是一个受限版本，接口受限。
// 不同之处在于您可以锁定和解锁std::unique_lock。std::lock_guard只在建造时锁定一次，在破坏时解锁。
// lock_guard始终持有一个锁，从构造到销毁。可以在不立即锁定的情况下创建unique_lock，可以在其存在的任何时候解锁，并且可以将锁的所有权从一个实例转移到另一个实例。
// 除非需要使用条件变量，这时需要unique_lock的功能。否则总是可以使用lock_guard
```

* condition_variable 头文件

```C++
#include <condition_variable>

// wait_for 函数，阻止某个线程，并设置线程阻止的时间间隔。
template <class Rep, class Period>
cv_status wait_for(
    unique_lock<mutex>& Lck,
    const chrono::duration<Rep, Period>& Rel_time);

// 枚举 cv_status, 该类型指示函数是否因为超时而返回。
enum class cv_status;
```

* chrono 头文件，时间库

```C++
#include <chrono>
// 这个头中的所有元素(除了common_type特殊化)不是直接在std名称空间下定义的，而是在std::chrono名称空间下定义的。
typedef duration<long long> seconds;

```

## 笔记

阻塞模板队列类，利用锁和条件变量对deque容器进行封装，实现对双端队列的阻塞访问。

私有成员变量：

* std::deque\<T> deq_：双端队列，存放具体内容
* size_t capacity_：阻塞队列的最大容量
* std::mutex mtx_：互斥锁，用于队列资源的互斥访问
* bool is_close_：标识阻塞队列是否关闭
* std::condition_variable cond_consumer_：用于消费者的条件变量，用于在队列为空的时候阻塞，不为空时访问
* std::condition_variable cond_producer_：用于生产者的条件变量，用于在队列满时阻塞，不满时访问

有参构造函数：传入参数为阻塞队列大小，初始化阻塞队列的最大容量，关闭标识置为false

析构函数：调用Close函数，关闭阻塞队列。

公有成员函数：

Clear、Front、Back、Size、Capacity、IsEmpty、IsFull的实现都是上锁后，调用容器的对应方法

Close：关闭阻塞队列，释放所用资源

* 对队列上锁后，清空队列内容，标识为关闭，释放条件变量

Flush：释放一个消费者条件变量，用于唤醒一个消费者

Clear：清空所有队列资源

Front：返回队列最前方元素

Back：返回队列最后方元素

Size：返回队列大小

Capacity：返回阻塞队列容量

PushBack：将一个item放到双端队列尾部

* 先上互斥锁
* 当队列已满，wait(locker)先阻塞这个线程，然后释放锁locker，直到收到notice后从此处运行
* 这里用while而不是if的原因是：在这个线程阻塞时，其他线程可能也会吧deq_填满。
* 通过条件变量后，将item添加到双端队列，并释放一个消费者线程

PushFront：将一个item放到双端队列头部，同PushBack

IsEmpty：判断双端队列是否为空

IsFull：判断双端队列释放已满

Pop(T &item)：当双端队列非空时，从双端队列头部弹出一个元素至参数item，否则阻塞

* 先上互斥锁
* 但队列为空，wait(locker)先阻塞这个线程，然后释放锁locker，直到收到notice后从此处运行
* 这里用while而不是if的原因是：在这个线程阻塞时，其他线程可能会把deq_取到空
* 通过条件变量后，首个元素复制给item，队列弹出首个元素，并唤一个生产者线程

Pop(T &item, int timeout)：当双端队列非空时，从双端队列头部弹出一个元素至参数item，否则阻塞timeout秒

* 与上一个Pop类似，不同之处在于，加了个判断，调用wait_for，设置阻塞时长为timeout秒，如果超时，则直接返回false
