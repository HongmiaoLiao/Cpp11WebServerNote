#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class Sem {
public:
    Sem() {
        if (sem_init(&sem, 0, 0) != 0) {
            throw std::exception();
        }
    }

    Sem(int num) {
        if (sem_init(&sem, 0, num) != 0) {
            throw std::exception();
        }
    }

    ~Sem() {
        sem_destroy(&sem);
    }

    bool wait() {
        return sem_wait(&sem) == 0;
    }

    bool post() {
        return sem_post(&sem) == 0;
    }

private:
    sem_t sem;
};

class Locker {
public:
    Locker() {
        if (pthread_mutex_init(&mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    ~Locker() {
        pthread_mutex_destroy(&mutex);
    }

    bool lock() {
        return pthread_mutex_lock(&mutex) == 0;
    }

    bool unlock() {
        return pthread_mutex_unlock(&mutex) == 0;
    }

    pthread_mutex_t* get() {
        return &mutex;
    }

private:
    pthread_mutex_t mutex;

};

class Cond {
public:
    Cond() {
        if (pthread_cond_init(&cond, NULL) != 0) {
            throw std::exception();
        }
    }

    ~Cond() {
        pthread_cond_destroy(&cond);
    }

    bool wait(pthread_mutex_t* mutex) {
        return pthread_cond_wait(&cond, mutex) == 0;
    }

    bool timeWait(pthread_mutex_t* mutex, struct timespec t) {
        return pthread_cond_timedwait(&cond, mutex, &t) == 0;
    }

    bool signal() {
        return pthread_cond_signal(&cond) == 0;
    }

    bool broadcast() {
        return pthread_cond_broadcast(&cond) == 0;
    }


private:
    pthread_cond_t cond;
};

#endif