#ifndef __WRAP_H__
#define __WRAP_H__

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include <exception>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <string>
using std::cout;
using std::endl;
using std::exception;
using std::list;
using std::map;
using std::pair;
using std::queue;
using std::string;

// ��װϵͳ�쳣����
extern void sys_error(const char *str);
// �ź�����ʼ��
extern void Sem_init(sem_t *sem, int pshared, unsigned int value);
// �ź�������
extern void Sem_destroy(sem_t *sem);
// �ź�������
extern bool Sem_wait(sem_t *sem);
// �ź�������
extern bool Sem_post(sem_t *sem);
// �̻߳�������ʼ��
extern void Pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
// �̻߳���������
extern void Pthread_mutex_destroy(pthread_mutex_t *mutex);
// �̻߳���������
extern bool Pthread_mutex_lock(pthread_mutex_t *mutex);
// �̻߳���������
extern bool Pthread_mutex_unlock(pthread_mutex_t *mutex);
// �߳�����������ʼ��
extern void Pthread_cond_init(pthread_cond_t *__restrict__ cond, const pthread_condattr_t *__restrict__ cond_attr);
// �߳�������������
extern void Pthread_cond_destroy(pthread_cond_t *cond);
// �߳��������������ȴ�
extern bool Pthread_cond_wait(pthread_cond_t *__restrict__ cond, pthread_mutex_t *__restrict__ mutex);
// �߳�����������ʱ�����ȴ�
extern bool Pthread_cond_timedwait(pthread_cond_t *__restrict__ cond, pthread_mutex_t *__restrict__ mutex, const timespec *__restrict__ abstime);
// �߳������������ѣ����ٻ���һ�������߳�
extern bool Pthread_cond_signal(pthread_cond_t *cond);
// �߳������������ѣ�����ȫ�������߳�
extern bool Pthread_cond_broadcast(pthread_cond_t *cond);
// ���ݿ��쳣������
extern void Mysql_error(MYSQL *mysql);
// ��ʼ�����Ӿ��
extern MYSQL *Mysql_init(MYSQL *mysql);
// ��������
extern MYSQL *Mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag);
// �����߳�
extern bool Pthread_create(pthread_t *__restrict__ newthread, const pthread_attr_t *__restrict__ attr, void *(*start_routine)(void *), void *__restrict__ arg);

#endif