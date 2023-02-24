#include "block_queue.h"

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
BlockQueue<T>::~BlockQueue() {
    mutex.lock();
    if (array != NULL ) {
        delete[] array;
    }
    mutex.unlock();
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
bool BlockQueue<T>::full() {
    mutex.lock();
    if (size > maxSize) {
        mutex.unlock();
        return true;
    }
    mutex.unlock();
    return false;
}

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
        if (cond.wait(mutex.get())) {
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
        if (cond.timeWait(mutex.get(), t)) {
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

