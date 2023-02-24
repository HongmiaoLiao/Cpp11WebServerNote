/*************************************************************
*循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;
*线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
**************************************************************/
// !!!!!!!!!!!!!!!!!!!!!!!!!
// 模板类的实现要和声明放在同一个文件之中！

// !!!!!!!!!!!!
#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"

using namespace std;
template <typename T>
class BlockQueue {
public:
    // 初始化私有成员
    BlockQueue(int maxSize);
    ~BlockQueue();
    void clear();
    // 判断队列是否满了
    bool full();
    // 判断队列是否为空
    bool empty();
    // 返回队首元素
    bool front(T& value);
    // 返回队尾元素
    bool back(T& value);
    int getSize();
    int getMaxSize();
    // 往队列添加元素，需要将所有使用队列的线程先唤醒
    // 当有元素push进队列，相当于生产者产生了一个元素
    // 若当前线程等待条件变量，则唤醒无意义
    bool push(const T& item);
    // pop时，如果当前队列没有元素，将会等待条件变量
    bool pop(T& item);
    // 增加超时处理
    bool pop(T& item, int msTimeout);

private:
    Locker mutex;
    Cond cond;

    T* array;
    int size;
    int maxSize;
    int frontIdx;
    int backIdx;

};

template <typename T>
BlockQueue<T>::BlockQueue(int maxSize) {
    if (maxSize <= 0) {
        exit(-1);
    }
    // 构造函数创建循环数组
    this->maxSize = maxSize;
    array = new T[maxSize];
    size = 0;
    frontIdx = -1;
    backIdx = -1;
}

template <typename T>
void BlockQueue<T>::clear() {
    mutex.lock();
    size = 0;
    frontIdx = -1;
    backIdx = -1;
    mutex.unlock();
}

template <typename T>
BlockQueue<T>::~BlockQueue() {
    mutex.lock();
    if (array != NULL ) {
        delete[] array;
    }
    mutex.unlock();
}

// 判断队列是否满
template <typename T>
bool BlockQueue<T>::full() {
    mutex.lock();
    if (size >= maxSize) {
        mutex.unlock();
        return true;
    }
    mutex.unlock();
    return false;
}

// 判断队列是否为空
template <typename T>
bool BlockQueue<T>::empty() {
    mutex.lock();
    if (size == 0) {
        mutex.lock();
        return true;
    }
    mutex.unlock();
    return false;
}

// 返回队首元素
template <typename T>
bool BlockQueue<T>::front(T& value) {
    mutex.lock();
    if (size == 0) {
        mutex.unlock();
        return false;
    }
    value = array[frontIdx];
    mutex.unlock();
    return true;
}

// 返回队尾元素
template <typename T>
bool BlockQueue<T>::back(T& value) {
    mutex.lock();
    if (size == 0) {
        mutex.unlock();
        return false;
    }
    value = array[backIdx];
    mutex.unlock();
    return true;
}

template <typename T>
int BlockQueue<T>::getSize() {
    int tmp = 0;
    mutex.lock();
    tmp = size;
    mutex.unlock();
    return tmp;
}

template <typename T>
int BlockQueue<T>::getMaxSize() {
    int tmp = 0;
    mutex.lock();
    tmp = maxSize;
    mutex.unlock();
    return tmp;
}


//往队列添加元素，需要将所有使用队列的线程先唤醒
//当有元素push进队列,相当于生产者生产了一个元素
//若当前没有线程等待条件变量,则唤醒无意义
template <typename T>
bool BlockQueue<T>::push(const T& item) {
    mutex.lock();
    if (size >= maxSize) {
        cond.broadcast();
        mutex.unlock();
        return false;
    }
    // 将新增数据放在循环数组的对应位置
    backIdx = (backIdx + 1) % maxSize;
    array[backIdx] = item;
    ++size;
    cond.broadcast();
    mutex.unlock();
    return true;
}

template <typename T>
bool BlockQueue<T>::pop(T& item) {
    mutex.lock();
    // 多个消费者的时候，这里要用while而不是if
    while (size <= 0) {
        // 当重新抢到互斥锁，cond.wait返回0
        if (!cond.wait(mutex.get())) {
            mutex.unlock();
            return false;
        }
    }
    // 取出队列首的元素，使用循环数组模拟的队列
    frontIdx = (frontIdx + 1) % maxSize;
    item = array[frontIdx];
    --size;
    mutex.unlock();
    return true;
}

template <typename T>
bool BlockQueue<T>::pop(T& item, int msTimeout) {
    struct timespec t = {0, 0};
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    mutex.lock();
    if (size <= 0) {
        t.tv_sec = now.tv_sec + msTimeout / 1000;
        t.tv_nsec = (msTimeout % 1000) * 1000;
        if (!cond.timeWait(mutex.get(), t)) {
            mutex.unlock();
            return false;
        }
    }
    if (size <= 0) {
        mutex.unlock();
        return false;
    }

    frontIdx = (frontIdx + 1) % maxSize;
    item = array[frontIdx];
    --size;
    mutex.unlock();
    return true;

}


#endif

// /*************************************************************
//  *循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;
//  *线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
//  **************************************************************/
// #ifndef BLOCK_QUEUE_H
// #define BLOCK_QUEUE_H

// #include <iostream>
// #include <stdlib.h>
// #include <pthread.h>
// #include <sys/time.h>
// #include "../lock/locker.h"

// using namespace std;
// template <typename T>
// class BlockQueue {
// public:
//     // 初始化私有成员
//     BlockQueue(int maxSize) {
//         if (maxSize <= 0) {
//             exit(-1);
//         }
//         // 构造函数创建循环数组
//         this->maxSize = maxSize;
//         array = new T[maxSize];
//         size = 0;
//         frontIdx = -1;
//         backIdx = -1;
//     }
//     ~BlockQueue() {
//         mutex.lock();
//         if (array != NULL ) {
//             delete[] array;
//         }
//         mutex.unlock();
//     }

//     void clear() {
//         mutex.lock();
//         size = 0;
//         frontIdx = -1;
//         backIdx = -1;
//         mutex.unlock();
//     }
    
//     // 判断队列是否满了
//     bool full() {
//         mutex.lock();
//         if (size > maxSize) {
//             mutex.unlock();
//             return true;
//         }
//         mutex.unlock();
//         return false;
//     }

//     // 判断队列是否为空
//     bool empty(){
//         mutex.lock();
//         if (size == 0) {
//             mutex.lock();
//             return true;
//         }
//         mutex.unlock();
//         return false;
//     }

//     // 返回队首元素
//     bool front(T &value) {
//         mutex.lock();
//         if (size == 0) {
//             mutex.unlock();
//             return false;
//         }
//         value = array[frontIdx];
//         mutex.unlock();
//         return true;
//     }

//     // 返回队尾元素
//     bool back(T &value) {
//         mutex.lock();
//         if (size == 0) {
//             mutex.unlock();
//             return false;
//         }
//         value = array[backIdx];
//         mutex.unlock();
//         return true;
//     }

//     int getSize() {
//         int tmp = 0;
//         mutex.lock();
//         tmp = size;
//         mutex.unlock();
//         return tmp;
//     }

//     int getMaxSize() {
//         int tmp = 0;
//         mutex.lock();
//         tmp = maxSize;
//         mutex.unlock();
//         return tmp;
//     }

//     // 往队列添加元素，需要将所有使用队列的线程先唤醒
//     // 当有元素push进队列，相当于生产者产生了一个元素
//     // 若当前线程等待条件变量，则唤醒无意义
//     bool push(const T &item) {
//         mutex.lock();
//         if (size >= maxSize) {
//             cond.broadcast();
//             mutex.unlock();
//             return false;
//         }
//         // 将新增数据放在循环数组的对应位置
//         backIdx = (backIdx + 1) % maxSize;
//         array[backIdx] = item;
//         ++size;
//         cond.broadcast();
//         mutex.unlock();
//         return true;
//     }

//     // pop时，如果当前队列没有元素，将会等待条件变量
//     bool pop(T &item) {
//         mutex.lock();
//         // 多个消费者的时候，这里要用while而不是if
//         while (size <= 0) {
//             // 当重新抢到互斥锁，cond.wait返回0
//             if (cond.wait(mutex.get())) {
//                 mutex.unlock();
//                 return false;
//             }
//         }
//         // 取出队列首的元素，使用循环数组模拟的队列
//         frontIdx = (frontIdx + 1) % maxSize;
//         item = array[frontIdx];
//         --size;
//         mutex.unlock();
//         return true;
//     }

//     // 增加超时处理
//     bool pop(T &item, int msTimeout) {
//         struct timespec t = {0, 0};
//         struct timeval now = {0, 0};
//         gettimeofday(&now, NULL);
//         mutex.lock();
//         if (size <= 0) {
//             t.tv_sec = now.tv_sec + msTimeout / 1000;
//             t.tv_nsec = (msTimeout % 1000) * 1000;
//             if (cond.timeWait(mutex.get(), t)) {
//                 mutex.unlock();
//                 return false;
//             }
//         }
//         if (size <= 0) {
//             mutex.unlock();
//             return false;
//         }

//         frontIdx = (frontIdx + 1) % maxSize;
//         item = array[frontIdx];
//         --size;
//         mutex.unlock();
//         return true;
//     }

// private:
//     Locker mutex;
//     Cond cond;

//     T *array;
//     int size;
//     int maxSize;
//     int frontIdx;
//     int backIdx;
// };

// #endif