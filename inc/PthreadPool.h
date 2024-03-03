#ifndef __PTHREADPOOL_H__
#define __PTHREADPOOL_H__

#include "Locker.h"
#include "SqlConnectionPool.h"
#include "wrap.h"

template <class T>
class PthreadPool {
private:
    int myPthreadNumber;                    // �̳߳��е��߳���
    int myMaxRequests;                      // ���������
    pthread_t *myPthreadPool;               // �̳߳�����
    list<T *> myRequestQueue;               // �������
    Mutex myMutex;                          // ����������еĻ�����
    Sem mySem;                              // �Ƿ�������Ҫ������ź���
    bool myStop;                            // �Ƿ�����߳�
    SqlConnectionPool *mySqlConnectionPool; // ���ݿ����ӳ�

private:
    // �����߳����к���
    static void *worker(void *arg);
    void run();

public:
    // �̳߳ع����ʼ��
    PthreadPool(SqlConnectionPool *sqlConnectionPool, int pthreadNumber = 8, int maxRequests = 10000);
    ~PthreadPool();
    // �����������
    bool append(T *request);
};

template <class T>
PthreadPool<T>::PthreadPool(SqlConnectionPool *sqlConnectionPool, int pthreadNumber, int maxRequests)
    : myPthreadNumber(pthreadNumber),
      myMaxRequests(maxRequests),
      myStop(false), // Ĭ�ϳ�ʼ��Ϊfalse����ʾ��ֹͣ�����߳�
      myPthreadPool(NULL),
      mySqlConnectionPool(sqlConnectionPool) {
    if (pthreadNumber <= 0 || maxRequests <= 0) {
        throw exception();
    }
    myPthreadPool = new pthread_t[pthreadNumber]; // �����̳߳�����
    if (myPthreadPool == nullptr) {
        throw exception();
    }
    for (int i = 0; i < pthreadNumber; ++i) {
        if (!Pthread_create(myPthreadPool + i, NULL, worker, this)) { // ���������̣߳������̳߳�
            delete[] myPthreadPool;
            throw exception();
        }
        if (pthread_detach(myPthreadPool[i])) { // �����̷߳���
            delete[] myPthreadPool;
            throw exception();
        }
    }
}

template <class T>
PthreadPool<T>::~PthreadPool() {
    if (myPthreadPool != nullptr) {
        delete[] myPthreadPool; // �����̳߳�
    }
    myStop = true; // ����Ϊtrue��ʹ�����߳�ֹͣ����
}

template <class T>
bool PthreadPool<T>::append(T *request) {
    myMutex.lock();
    if (myRequestQueue.size() >= myMaxRequests) {
        myMutex.unlock();
        return false;
    }
    myRequestQueue.push_back(request); // �����������
    myMutex.unlock();
    mySem.post(); // ֪ͨ�����߳�
    return true;
}

// �̻߳ص��������ڲ��������к���
template <class T>
void *PthreadPool<T>::worker(void *arg) {
    PthreadPool *pthreadPool = (PthreadPool *)arg;
    pthreadPool->run();
    return pthreadPool;
}

template <class T>
void PthreadPool<T>::run() {
    while (!myStop) {
        mySem.wait(); // ���������߳�
        myMutex.lock();
        if (myRequestQueue.empty()) {
            myMutex.unlock();
            continue;
        }
        // ����������ó�һ������
        T *request = myRequestQueue.front();
        myRequestQueue.pop_front();
        myMutex.unlock();
        if (request == NULL) {
            continue;
        }
        SqlConnectionPoolRAII sqlConnectionPoolRAII(&request->connection, mySqlConnectionPool); // �����ݿ����ӳ����ó�һ�����ݿ�����
        request->process();                                                                     // ��������Ĵ�����
    }
}

#endif