#ifndef __LOCKER_H__
#define __LOCKER_H__

#include "wrap.h"

/*封装信号量*/
class Sem {
private:
    sem_t mySem;

public:
    Sem() {
        Sem_init(&mySem, 0, 1);
    }
    Sem(int num) {
        Sem_init(&mySem, 0, num);
    }
    ~Sem() {
        Sem_destroy(&mySem);
    }
    bool wait() {
        return Sem_wait(&mySem);
    }
    bool post() {
        return Sem_post(&mySem);
    }
};

/*封装互斥量*/
class Mutex {
private:
    pthread_mutex_t myMutex;

public:
    Mutex() {
        Pthread_mutex_init(&myMutex, NULL);
    }
    ~Mutex() {
        Pthread_mutex_destroy(&myMutex);
    }
    bool lock() {
        return Pthread_mutex_lock(&myMutex);
    }
    bool unlock() {
        return Pthread_mutex_unlock(&myMutex);
    }
    pthread_mutex_t *getMyMutex() {
        return &myMutex;
    }
};

/*封装条件变量*/
class Cond {
private:
    pthread_cond_t myCond;

public:
    Cond() {
        Pthread_cond_init(&myCond, NULL);
    }
    ~Cond() {
        Pthread_cond_destroy(&myCond);
    }
    bool wait(pthread_mutex_t *myMutex) {
        return Pthread_cond_wait(&myCond, myMutex);
    }
    bool timewait(pthread_mutex_t *myMutex, struct timespec time) {
        return Pthread_cond_timedwait(&myCond, myMutex, &time);
    }
    bool signal() {
        return Pthread_cond_signal(&myCond);
    }
    bool broadcast() {
        return Pthread_cond_broadcast(&myCond);
    }
};

#endif