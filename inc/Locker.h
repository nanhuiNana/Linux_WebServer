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
            perror("sem_init error");
            exit(-1);
        }
    }
    Sem(int num) {
        if (sem_init(&mySem, 0, num) == -1) {
            perror("sem_init_int error");
            exit(-1);
        }
    }
    ~Sem() {
        if (sem_destroy(&mySem) == -1) {
            perror("sem_destroy error");
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
            perror("mutex_init error");
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
};

#endif