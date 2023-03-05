# pool/threadpool.h

## 使用库

* mutex头文件，具有允许代码关键部分并发执行互斥的功能，允许显式地避免数据争用。

```C++
#include <mutex>

// mutex 类，表示互斥体类型。 此类型的对象可用于强制程序内的互斥。
class mutex;

// unique_lock 类，表示可进行实例化以创建管理mutex锁定和解锁的对象的模板。
template <class Mutex>
class unique_lock;
// 模板参数 Mutex 必须为 mutex 类型。

// mutex创建后需要手动调用mtx.lock()和mtx.unlock()
// unique_lock在构造函数中调用lock，在析构函数中对互斥量调用unlock。（当然也可以手动调用）
// 使用unique_lock的好处是，如果抛出一些异常，您可以确定互斥锁在离开定义unique_lock的作用域时将其解锁。

```

* condition_variable头文件，声明条件变量类型，类似信号量:

```C++
#include <condition_variable>

// condition_variable 类，使用 condition_variable 类在具有 mutex 类型的 unique_lock<mutex> 时等待事件。
class condition_variable;

// condition_variable 成员函数 wait，阻塞线程。在阻塞线程的时刻，函数自动调用lck.unlock()，允许其他被锁定的线程继续。
void wait(unique_lock<mutex>& Lck);

// 等待条件变量的代码必须也使用 mutex。 调用线程必须在其调用等待条件变量的函数前锁定 mutex。 
// 然后，mutex 将在所调用的函数返回时被锁定。 mutex 在线程等待条件变为 true 时不被锁定。 因此，没有不可预知的结果，等待条件变量的每个线程必须使用同一 mutex 对象。
// condition_variable_any 类型的对象可与任何类型的 mutex 一起使用。 所使用的 mutex 类型不一定提供 try_lock 方法。 
// condition_variable 类型的对象只能与 unique_lock<mutex> 类型的 mutex 一起使用。 此类型的对象可能比 condition_variable_any<unique_lock<mutex>> 类型的对象快。

while (condition is false)
    wait for condition variable;



// condition_variable 成员函数 notice_one，取消阻塞正在 condition_variable 对象上等待的某个线程。
void notify_one() noexcept;

// wait就是P操作，信号量减1，notice_one就是V操作，信号量加1

// condition_variable 成员函数 notice_all，取消阻塞正在等待 condition_variable 对象的所有线程。
void notify_all() noexcept;
```

* queue头文件，定义了队列queue和优先队列（堆）priority_queue容器适配器类，不多赘述

* thread头文件，声明线程类和this_thread命名空间:

```C++
#include <thread>

// thread 类，定义用于查看和管理应用程序中执行线程的对象。
class thread;

// thread 构造函数
template <class Fn, class... Args>
explicit thread(Fn&& F, Args&&... A);
// F：要由线程执行的应用程序所定义的函数。
// A：要传递到 F 的参数列表。

// thread 类成员函数 detach，拆离相关联的线程。操作系统负责释放终止的线程资源。
void detach();
```

* functional头文件，函数对象（仿函数）是专门设计用于使用类似于函数的语法的对象。在c++中，这是通过在类中定义成员函数operator()（重载括号运算符）来实现

```C++
#include <functional>

// function 类，可调用对象的包装器。
template <class Fty>
class function
// Fty：要包装的函数类型
```

* cassert头文件（C诊断库assert.h），定义了一个宏函数，可以用作标准的调试工具:

```C++
#include <cassert>

// assert 宏，计算表达式，如果结果为 false，则打印诊断消息并中止程序。
// 语法：
assert(
   expression
);
```

* memory头文件，定义了管理动态内存的通用工具

```C++
#include <memory>

// shared_ptr 类，使用引用计数来管理资源的对象
template <class T>
class shared_ptr;
// 如果你想要将一个原始指针分配给多个所有者（例如，从容器返回了指针副本又想保留原始指针时），请使用该指针。 直至所有 shared_ptr 所有者超出了范围或放弃所有权，才会删除原始指针。 大小为两个指针；一个用于对象，另一个用于包含引用计数的共享控制块。

// make_shared 函数，创建并返回指向分配对象的 shared_ptr，这些对象是通过使用默认分配器从零个或多个参数构造的。 分配并构造指定类型的对象和shared_ptr来管理对象的共享所有权，并返回shared_ptr。
template <class T, class... Args>
shared_ptr<T> make_shared(
    Args&&... args);
```

* utility头文件，定义有助于构造和管理对象对的 C++ 标准库类型、函数和运算符

```C++
#include <utility>

// move 函数，无条件将其自变量强制转换为右值引用，从而表示其可以移动（如果其类型支持移动）。
template <class Type>
    constexpr typename remove_reference<Type>::type&& move(Type&& Arg) noexcept;

// forward 函数，如果自变量是右值或右值引用，则有条件地将其自变量强制转换为右值引用。 这会将自变量的右值状态还原到转发函数，以支持完美转发。
template <class Type>    // accepts lvalues
    constexpr Type&& forward(typename remove_reference<Type>::type& Arg) noexcept

template <class Type>    // accepts everything else
    constexpr Type&& forward(typename remove_reference<Type>::type&& Arg) noexcept

```

* 其他

```C++
// static_cast 运算符,仅根据表达式中存在的类型，将 expression 转换为 type-id 类型。
static_cast <type-id> ( expression )

// dynamic_cast 运算符，将操作数 expression 转换为 type-id 类型的对象。
dynamic_cast < type-id > ( expression )

// 当从基类类型转换为派生类类型时，请使用dynamic_cast。它检查被强制转换的对象实际上是派生类类型，如果对象不是所需类型(除非您强制转换为引用类型——然后抛出bad_cast异常)则返回空指针。
// 如果不需要这个额外的检查，请使用static_cast。由于dynamic_cast执行额外的检查，它需要RTTI信息，因此有更大的运行时开销，而static_cast在编译时执行。通常使用 static_cast 转换数值数据类型

// size_t是sizeof、_Alignof(自C11起)和offsetof的结果的无符号整数类型，取决于数据模型。
size_t
```

## 笔记记录

线程池类，用于线程的创建，任务的添加

私有成员变量：

* std::shared_ptr\<Pool> pool_，一个线程池的智能指针
  * 将互斥锁、条件遍历、关闭标识以及任务队列放在一个结构体内，由智能指针管理任务队列资源

有参构造函数：

* 传入参数为线程池允许的线程数量，默认为8
* 线程池构造函数使用关键字explicit阻止构造函数在隐式转换中使用
* 初始化列表调用make_shared函数，创建线程池结构体，并给智能指针赋值
* 使用assert确保线程池线程总量要大于0
* 使用一个循环逐个初始化所有线程
  * 创建thread对象，构造函数传入参数为一个可调用对象，即一个lambda创建的仿函数，捕获任务队列结构体的智能指针。这个仿函数执行过程就是一个执行的线程，线程工作为：
    * 创建一个独占锁对象，用于给线程的执行上锁，使各线程互斥取出队列中的任务。
    * 进入一个无限循环，不断从取出任务给线程执行
      * 如果任务队列，非空，则从队列中取出任务，取出的任务对象（仿函数）采用移动赋值，更高效
      * 如果线程池关闭，则结束循环，退出线程
      * 线程池为空，使用条件变量阻塞一个线程池中的线程
  * 将线程交给操作系统管理资源

无参构造函数，使用=default关键字，令编译器自动生成（没有创建线程和任务队列）

移动构造函数，使用=default关键字，令编译器自动生成

析构函数：

* 如果任务队列结构体未被释放
  * 将队列资源上锁，将是否关闭标识置为true，注意，这里的大括号作用域不是多余的，因为这样可以让locker在退出这个作用域就执行析构函数释放锁，而不需要外层作用域结束才是否，效率更高
  * 取消阻塞正在等待条件变量的所有线程

成员函数AddTask，作用是将要执行的任务放进任务队列中

* 是一个模板成员函数，模板类型应该为一个仿函数，传入参数为执行任务的仿函数。
  * 将任务队列上锁。这里大括号作用域同析构函数所述。
  * 将任务加入任务队列，这里使用的是forward函数而不是move，因为Rvalue经由一个接收参数为T&&类型的函数接收后，转发过程这个Rvalue变成了一个named object，不再是Rvalue（侯捷老师在C++新特性课程讲过），使用forward可以完美转发
  * 取消阻塞正在等待条件变量的一个线程（让它去执行任务）

## 测试

注意在编译选项加上 -pthread

```C++
#include <iostream>
#include "./pool/threadpool.h"
#include <ctime>

int main() {
    ThreadPool threadPool(8);
    for (int i = 0; i < 100; ++i) {
        threadPool.AddTask([i](){
           std::cout << time(nullptr) << "----" << i << std::endl;
        });
    }
    std::cout << "Hello, World!" << std::endl;
    while(true) {

    }
    return 0;
}
```

防止主程序过快结束，就加了一个死循环，因为是多线程，所以输出格式会有一些地方出现混乱，输出内容没有问题
