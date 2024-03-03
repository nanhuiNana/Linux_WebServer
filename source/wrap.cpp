#include "../inc/wrap.h"

void sys_error(const char *str) {
    perror("str");
    exit(-1);
}

void Sem_init(sem_t *sem, int pshared, unsigned int value) {
    if (sem_init(sem, pshared, value) == -1) {
        sys_error("sem init error");
    }
}

void Sem_destroy(sem_t *sem) {
    if (sem_destroy(sem) == -1) {
        sys_error("sem destroy error");
    }
}

bool Sem_wait(sem_t *sem) {
    return sem_wait(sem) == 0;
}

bool Sem_post(sem_t *sem) {
    return sem_post(sem) == 0;
}

void Pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
    if (pthread_mutex_init(mutex, mutexattr) != 0) {
        sys_error("mutex init error");
    }
}

void Pthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (pthread_mutex_destroy(mutex) != 0) {
        sys_error("mutex destroy error");
    }
}

bool Pthread_mutex_lock(pthread_mutex_t *mutex) {
    return pthread_mutex_lock(mutex) == 0;
}

bool Pthread_mutex_unlock(pthread_mutex_t *mutex) {
    return pthread_mutex_unlock(mutex) == 0;
}

void Pthread_cond_init(pthread_cond_t *__restrict__ cond, const pthread_condattr_t *__restrict__ cond_attr) {
    if (pthread_cond_init(cond, cond_attr) != 0) {
        sys_error("cond init error");
    }
}

void Pthread_cond_destroy(pthread_cond_t *cond) {
    if (pthread_cond_destroy(cond) != 0) {
        sys_error("cond destroy error");
    }
}

bool Pthread_cond_wait(pthread_cond_t *__restrict__ cond, pthread_mutex_t *__restrict__ mutex) {
    return pthread_cond_wait(cond, mutex) == 0;
}

bool Pthread_cond_timedwait(pthread_cond_t *__restrict__ cond, pthread_mutex_t *__restrict__ mutex, const timespec *__restrict__ abstime) {
    return pthread_cond_timedwait(cond, mutex, abstime) == 0;
}

bool Pthread_cond_signal(pthread_cond_t *cond) {
    return pthread_cond_signal(cond) == 0;
}

bool Pthread_cond_broadcast(pthread_cond_t *cond) {
    return pthread_cond_broadcast(cond) == 0;
}

void Mysql_error(MYSQL *mysql) {
    cout << "MYSQL ERROR: " << mysql_error(mysql) << endl;
    exit(-2);
}

MYSQL *Mysql_init(MYSQL *mysql) {
    mysql = mysql_init(mysql);
    if (mysql == NULL) {
        Mysql_error(mysql);
    }
    return mysql;
}

MYSQL *Mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag) {
    mysql = mysql_real_connect(mysql, host, user, passwd, db, port, unix_socket, clientflag);
    if (mysql == NULL) {
        Mysql_error(mysql);
    }
    return mysql;
}

bool Pthread_create(pthread_t *__restrict__ newthread, const pthread_attr_t *__restrict__ attr, void *(*start_routine)(void *), void *__restrict__ arg) {
    return pthread_create(newthread, attr, start_routine, arg) == 0;
}
