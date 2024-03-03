#ifndef __TIMER_H__
#define __TIMER_H__

#include "Log.h"
#include "wrap.h"

class Timer;

// 连接资源定义
struct ClientData {
    struct sockaddr_in clientAddr; // 客户端地址
    int socketFd;                  // 通信套接字
    Timer *timer;                  // 定时器
};

// 定时器类定义
class Timer {
public:
    time_t overtime;                 // 超时时间
    ClientData *userData;            // 连接资源
    void *callbackFun(ClientData *); // 回调函数
    Timer *prev;                     // 前置指针
    Timer *next;                     // 后置指针
    Timer() : prev(NULL), next(NULL) {}
};

// 定时器升序链表定义（定时器容器）
class TimerSortList {
private:
    Timer *head;
    Timer *tail;

public:
    TimerSortList() : head(NULL), tail(NULL) {}
    ~TimerSortList() {
        Timer *temp = head;
        // 循环delete定时器结点
        while (temp) {
            head = temp->next;
            delete temp;
            temp = head;
        }
    }
    // 添加定时器结点
    void addTimer(Timer *timer) {
        if (timer == NULL) {
            return;
        }
        if (head == NULL) { // 头结点为空
            head = tail = timer;
            return;
        }
        if (timer->overtime < head->overtime) { // 定时器结点超时时间比头结点小
            timer->next = head;
            timer->prev = NULL;
            head->prev = timer;
            head = timer;
            return;
        }
        Timer *prev = head;
        Timer *temp = prev->next;
        while (temp) {
            if (timer->overtime < temp->overtime) { // 找到一个结点超时时间比定时器结点大
                prev->next = timer;
                timer->next = temp;
                timer->prev = prev;
                temp->prev = timer;
                break;
            }
            prev = temp;
            temp = prev->next;
        }
        if (temp == NULL) { // 所有结点超时时间都比定时器小
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }
    // 调整定时器结点
    void adjustTimer(Timer *timer) {
        if (timer == NULL) {
            return;
        }
        Timer *temp = timer->next;
        if (temp == NULL || timer->overtime < temp->overtime) { // 定时器结点的下一个结点为空或定时器超时时间小于下一个结点，不需要调整
            return;
        }
        if (timer == head) { // 如果定时器结点是头结点，对其进行调整，将其从链表上摘下，重新插入链表
            head = head->next;
            timer->next = NULL;
            addTimer(timer);
            return;
        }
        // 与头结点同理，对其进行调整，摘下再重新插入
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        addTimer(timer);
    }
    // 删除定时器结点
    void deleteTimer(Timer *timer) {
        if (timer == NULL) {
            return;
        }
        if (timer == head && timer == tail) { // 定时器结点既是头结点也是尾结点
            delete timer;
            head = tail = NULL;
            return;
        }
        if (timer == head) { // 定时器结点是头结点
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }
        if (timer == tail) { // 定时器结点是尾结点
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }
        // 定时器不是头也不是尾结点
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    // 定时器触发函数(心搏函数)
    void tick() {
        // 定时器会按照设定的超时时间依次触发对应的回调函数,同时写入日志，并在触发后从链表中移除该定时器节点。如果链表为空，定时器将不执行任何操作。
        if (head == NULL) {
            return;
        }
        LOG_INFO("%s", "timer tick");
        Log::getInstance()->flush();
        time_t now = time(NULL);
        Timer *temp = head;
        while (temp) {
            if (now < temp->overtime) {
                break;
            }
            temp->callbackFun(temp->userData);
            head = temp->next;
            if (head) {
                head->prev = NULL;
            }
            delete temp;
            temp = head;
        }
    }
};

#endif