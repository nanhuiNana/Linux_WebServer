#ifndef __LOCKER_H__
#define __LOCKER_H__

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

/*封装信号量*/
class Sem {
private:
    sem_t mySem;

public:
    Sem() {
        if (sem_init(&mySem, 0, 1) == -1) {
            perror("sem init error");
            exit(-1);
        }
    }
    Sem(int num) {
        if (sem_init(&mySem, 0, num) == -1) {
            perror("sem init_int error");
            exit(-1);
        }
    }
    ~Sem() {
        if (sem_destroy(&mySem) == -1) {
            perror("sem destroy error");
            exit(-1);
        }
    }
    bool wait() {
        return sem_wait(&mySem) == 0;
    }
    bool post() {
        return sem_post(&mySem) == 0;
    }
};

/*封装互斥量*/
class Mutex {
private:
    pthread_mutex_t myMutex;

public:
    Mutex() {
        if (pthread_mutex_init(&myMutex, NULL) != 0) {
            perror("mutex init error");
            exit(-1);
        }
    }
    ~Mutex() {
        if (pthread_mutex_destroy(&myMutex) != 0) {
            perror("mutex destroy error");
            exit(-1);
        }
    }
    bool lock() {
        return pthread_mutex_lock(&myMutex) == 0;
    }
    bool unlock() {
        return pthread_mutex_unlock(&myMutex) == 0;
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
        if (pthread_cond_init(&myCond, NULL) != 0) {
            perror("cond init error");
        }
    }
    ~Cond() {
        if (pthread_cond_destroy(&myCond) != 0) {
            perror("cond destroy error");
        }
    }
    bool wait(pthread_mutex_t *myMutex) {
        return pthread_cond_wait(&myCond, myMutex) == 0;
    }
    bool timewait(pthread_mutex_t *myMutex, struct timespec time) {
        return pthread_cond_timedwait(&myCond, myMutex, &time) == 0;
    }
    bool signal() {
        return pthread_cond_signal(&myCond) == 0;
    }
    bool broadcast() {
        return pthread_cond_broadcast(&myCond) == 0;
    }
};

#endif