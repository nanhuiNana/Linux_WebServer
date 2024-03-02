#ifndef __WRAP_H__
#define __WRAP_H__

#include <errno.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <list>
#include <queue>
#include <string>
using std::cout;
using std::endl;
using std::list;
using std::queue;
using std::string;

// 信号量初始化
extern void Sem_init(sem_t *sem, int pshared, unsigned int value);
// 信号量销毁
extern void Sem_destroy(sem_t *sem);
// 信号量加锁
extern bool Sem_wait(sem_t *sem);
// 信号量解锁
extern bool Sem_post(sem_t *sem);
// 线程互斥量初始化
extern void Pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
// 线程互斥量销毁
extern void Pthread_mutex_destroy(pthread_mutex_t *mutex);
// 线程互斥量加锁
extern bool Pthread_mutex_lock(pthread_mutex_t *mutex);
// 线程互斥量解锁
extern bool Pthread_mutex_unlock(pthread_mutex_t *mutex);
// 线程条件变量初始化
extern void Pthread_cond_init(pthread_cond_t *__restrict__ cond, const pthread_condattr_t *__restrict__ cond_attr);
// 线程条件变量销毁
extern void Pthread_cond_destroy(pthread_cond_t *cond);
// 线程条件变量阻塞等待
extern bool Pthread_cond_wait(pthread_cond_t *__restrict__ cond, pthread_mutex_t *__restrict__ mutex);
// 线程条件变量限时阻塞等待
extern bool Pthread_cond_timedwait(pthread_cond_t *__restrict__ cond, pthread_mutex_t *__restrict__ mutex, const timespec *__restrict__ abstime);
// 线程条件变量唤醒，至少唤醒一个阻塞线程
extern bool Pthread_cond_signal(pthread_cond_t *cond);
// 线程条件变量唤醒，唤醒全部阻塞线程
extern bool Pthread_cond_broadcast(pthread_cond_t *cond);
// 初始化连接句柄
// extern MYSQL *Mysql_init(MYSQL *mysql);

#endif