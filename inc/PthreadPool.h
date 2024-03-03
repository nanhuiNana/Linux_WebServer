#ifndef __PTHREADPOOL_H__
#define __PTHREADPOOL_H__

#include "Locker.h"
#include "SqlConnectionPool.h"
#include "wrap.h"

template <class T>
class PthreadPool {
private:
    int myPthreadNumber;                    // 线程池中的线程数
    int myMaxRequests;                      // 最大请求数
    pthread_t *myPthreadPool;               // 线程池数组
    list<T *> myRequestQueue;               // 请求队列
    Mutex myMutex;                          // 保护请求队列的互斥量
    Sem mySem;                              // 是否有请求要处理的信号量
    bool myStop;                            // 是否结束线程
    SqlConnectionPool *mySqlConnectionPool; // 数据库连接池

private:
    // 工作线程运行函数
    static void *worker(void *arg);
    void run();

public:
    // 线程池构造初始化
    PthreadPool(SqlConnectionPool *sqlConnectionPool, int pthreadNumber = 8, int maxRequests = 10000);
    ~PthreadPool();
    // 添加请求任务
    bool append(T *request);
};

template <class T>
PthreadPool<T>::PthreadPool(SqlConnectionPool *sqlConnectionPool, int pthreadNumber, int maxRequests)
    : myPthreadNumber(pthreadNumber),
      myMaxRequests(maxRequests),
      myStop(false), // 默认初始化为false，表示不停止工作线程
      myPthreadPool(NULL),
      mySqlConnectionPool(sqlConnectionPool) {
    if (pthreadNumber <= 0 || maxRequests <= 0) {
        throw exception();
    }
    myPthreadPool = new pthread_t[pthreadNumber]; // 创建线程池数组
    if (myPthreadPool == nullptr) {
        throw exception();
    }
    for (int i = 0; i < pthreadNumber; ++i) {
        if (!Pthread_create(myPthreadPool + i, NULL, worker, this)) { // 创建工作线程，放入线程池
            delete[] myPthreadPool;
            throw exception();
        }
        if (pthread_detach(myPthreadPool[i])) { // 设置线程分离
            delete[] myPthreadPool;
            throw exception();
        }
    }
}

template <class T>
PthreadPool<T>::~PthreadPool() {
    if (myPthreadPool != nullptr) {
        delete[] myPthreadPool; // 销毁线程池
    }
    myStop = true; // 设置为true，使工作线程停止运行
}

template <class T>
bool PthreadPool<T>::append(T *request) {
    myMutex.lock();
    if (myRequestQueue.size() >= myMaxRequests) {
        myMutex.unlock();
        return false;
    }
    myRequestQueue.push_back(request); // 添加请求任务
    myMutex.unlock();
    mySem.post(); // 通知工作线程
    return true;
}

// 线程回调函数，内部调用运行函数
template <class T>
void *PthreadPool<T>::worker(void *arg) {
    PthreadPool *pthreadPool = (PthreadPool *)arg;
    pthreadPool->run();
    return pthreadPool;
}

template <class T>
void PthreadPool<T>::run() {
    while (!myStop) {
        mySem.wait(); // 阻塞工作线程
        myMutex.lock();
        if (myRequestQueue.empty()) {
            myMutex.unlock();
            continue;
        }
        // 从请求队列拿出一个请求
        T *request = myRequestQueue.front();
        myRequestQueue.pop_front();
        myMutex.unlock();
        if (request == NULL) {
            continue;
        }
        SqlConnectionPoolRAII sqlConnectionPoolRAII(&request->connection, mySqlConnectionPool); // 从数据库连接池中拿出一个数据库连接
        request->process();                                                                     // 调用请求的处理函数
    }
}

#endif