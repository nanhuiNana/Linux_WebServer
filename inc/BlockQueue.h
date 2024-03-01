#ifndef __BLOCKQUEUE_H__
#define __BLOCKQUEUE_H__

#include <./Locker.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#include <iostream>
#include <queue>
using std::queue;

/*
 * ʹ��STL�е�queue����ʵ����������
 * ʹ�û�������֤�̰߳�ȫ��ʹ����������ʵ���߳�ͬ��
 */
template <typename T>
class BlockQueue {
private:
    queue<T> myQueue;  // �������
    int maxSize;       // ���г������ֵ
    Mutex myMutex;     // ������
    Cond myCond;       // ��������

public:
    // ���췽���г�ʼ����Ա����
    BlockQueue(int num = 1000) : maxSize(num) {
        if (maxSize <= 0) {
            exit(-1);
        }
    }

    // ��ն���
    void clear() {
        myMutex.lock();
        myQueue.clear();
        myMutex.unlock();
    }

    // �ж϶����Ƿ�Ϊ��
    bool full() {
        myMutex.lock();
        if (myQueue.size() >= maxSize) {
            myMutex.unlock();
            return true;
        } else {
            myMutex.unlock();
            return false;
        }
    }

    // �ж϶����Ƿ�Ϊ��
    bool empty() {
        myMutex.lock();
        bool flag = myQueue.empty();
        myMutex.unlock();
        return flag;
    }

    // ��ȡ��ͷԪ�أ�ͨ������ֵ�ж��Ƿ�ɹ�
    bool front(T& value) {
        myMutex.lock();
        if (myQueue.empty()) {
            myMutex.unlock();
            return false;
        } else {
            value = myQueue.front();
            myMutex.unlock();
            return true;
        }
    }

    // ��ȡ��βԪ�أ�ͨ������ֵ�ж��Ƿ�ɹ�
    bool back(T& value) {
        myMutex.lock();
        if (myQueue.empty()) {
            myMutex.unlock();
            return false;
        } else {
            value = myQueue.back();
            myMutex.unlock();
            return true;
        }
    }

    // ��ȡ���г���
    int size() {
        int num = 0;
        myMutex.lock();
        num = myQueue.size();
        myMutex.unlock();
        return num;
    }

    // ��ȡ���г������ֵ
    int max_Size() {
        int num = 0;
        myMutex.lock();
        num = maxSize;
        myMutex.unlock();
        return num;
    }

    // ��Ӳ�����������������Ʒ�������������
    bool push(const T& value) {
        myMutex.lock();
        if (myQueue.size() >= maxSize) {
            myCond.broadcast();
            myMutex.unlock();
            return false;
        }
        myQueue.push(value);
        myCond.broadcast();
        myMutex.unlock();
        return true;
    }

    // ���Ӳ����������ߴ���������л�ȡ��Ʒ
    bool pop(T& value) {
        myMutex.lock();
        while (myQueue.size() <= 0) {
            if (!myCond.wait(myMutex.getMyMutex())) {
                myMutex.unlock();
                return false;
            }
        }
        value = myQueue.front();
        myQueue.pop();
        myMutex.unlock();
        return true;
    }

    // ���Ӳ������������߳���ӳ�ʱʱ��
    bool pop(T& value, int timeoutMS) {
        struct timespec timeout = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        myMutex.lock();
        while (myQueue.size() <= 0) {
            timeout.tv_sec = now.tv_sec + timeoutMS / 1000;
            timeout.tv_nsec = (timeoutMS % 1000) * 1000;
            if (!myCond.timewait(myMutex.getMyMutex(), timeout)) {
                myMutex.unlock();
                return false;
            }
            // ��������ж�һ�Σ��������������ֱ�ӷ���ʧ��
            if (myQueue.size() <= 0) {
                myMutex.unlock();
                return false;
            }
        }
        value = myQueue.front();
        myQueue.pop();
        myMutex.unlock();
        return true;
    }
};

#endif