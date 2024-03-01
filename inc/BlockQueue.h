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
 * 使用STL中的queue容器实现阻塞队列
 * 使用互斥锁保证线程安全，使用条件变量实现线程同步
 */
template <typename T>
class BlockQueue {
private:
    queue<T> myQueue;  // 任务队列
    int maxSize;       // 队列长度最大值
    Mutex myMutex;     // 互斥锁
    Cond myCond;       // 条件变量

public:
    // 构造方法中初始化成员变量
    BlockQueue(int num = 1000) : maxSize(num) {
        if (maxSize <= 0) {
            exit(-1);
        }
    }

    // 清空队列
    void clear() {
        myMutex.lock();
        myQueue.clear();
        myMutex.unlock();
    }

    // 判断队列是否为满
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

    // 判断队列是否为空
    bool empty() {
        myMutex.lock();
        bool flag = myQueue.empty();
        myMutex.unlock();
        return flag;
    }

    // 获取队头元素，通过返回值判断是否成功
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

    // 获取队尾元素，通过返回值判断是否成功
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

    // 获取队列长度
    int size() {
        int num = 0;
        myMutex.lock();
        num = myQueue.size();
        myMutex.unlock();
        return num;
    }

    // 获取队列长度最大值
    int max_Size() {
        int num = 0;
        myMutex.lock();
        num = maxSize;
        myMutex.unlock();
        return num;
    }

    // 入队操作，生产者生产产品放入任务队列中
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

    // 出队操作，消费者从任务队列中获取产品
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

    // 出队操作，给工作线程添加超时时长
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
            // 额外进行判断一次，如果不满足条件直接返回失败
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