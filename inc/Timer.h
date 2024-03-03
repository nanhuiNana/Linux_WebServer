#ifndef __TIMER_H__
#define __TIMER_H__

#include "Log.h"
#include "wrap.h"

class Timer;

// ������Դ����
struct ClientData {
    struct sockaddr_in clientAddr; // �ͻ��˵�ַ
    int socketFd;                  // ͨ���׽���
    Timer *timer;                  // ��ʱ��
};

// ��ʱ���ඨ��
class Timer {
public:
    time_t overtime;                 // ��ʱʱ��
    ClientData *userData;            // ������Դ
    void *callbackFun(ClientData *); // �ص�����
    Timer *prev;                     // ǰ��ָ��
    Timer *next;                     // ����ָ��
    Timer() : prev(NULL), next(NULL) {}
};

// ��ʱ�����������壨��ʱ��������
class TimerSortList {
private:
    Timer *head;
    Timer *tail;

public:
    TimerSortList() : head(NULL), tail(NULL) {}
    ~TimerSortList() {
        Timer *temp = head;
        // ѭ��delete��ʱ�����
        while (temp) {
            head = temp->next;
            delete temp;
            temp = head;
        }
    }
    // ��Ӷ�ʱ�����
    void addTimer(Timer *timer) {
        if (timer == NULL) {
            return;
        }
        if (head == NULL) { // ͷ���Ϊ��
            head = tail = timer;
            return;
        }
        if (timer->overtime < head->overtime) { // ��ʱ����㳬ʱʱ���ͷ���С
            timer->next = head;
            timer->prev = NULL;
            head->prev = timer;
            head = timer;
            return;
        }
        Timer *prev = head;
        Timer *temp = prev->next;
        while (temp) {
            if (timer->overtime < temp->overtime) { // �ҵ�һ����㳬ʱʱ��ȶ�ʱ������
                prev->next = timer;
                timer->next = temp;
                timer->prev = prev;
                temp->prev = timer;
                break;
            }
            prev = temp;
            temp = prev->next;
        }
        if (temp == NULL) { // ���н�㳬ʱʱ�䶼�ȶ�ʱ��С
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }
    // ������ʱ�����
    void adjustTimer(Timer *timer) {
        if (timer == NULL) {
            return;
        }
        Timer *temp = timer->next;
        if (temp == NULL || timer->overtime < temp->overtime) { // ��ʱ��������һ�����Ϊ�ջ�ʱ����ʱʱ��С����һ����㣬����Ҫ����
            return;
        }
        if (timer == head) { // �����ʱ�������ͷ��㣬������е����������������ժ�£����²�������
            head = head->next;
            timer->next = NULL;
            addTimer(timer);
            return;
        }
        // ��ͷ���ͬ��������е�����ժ�������²���
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        addTimer(timer);
    }
    // ɾ����ʱ�����
    void deleteTimer(Timer *timer) {
        if (timer == NULL) {
            return;
        }
        if (timer == head && timer == tail) { // ��ʱ��������ͷ���Ҳ��β���
            delete timer;
            head = tail = NULL;
            return;
        }
        if (timer == head) { // ��ʱ�������ͷ���
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }
        if (timer == tail) { // ��ʱ�������β���
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }
        // ��ʱ������ͷҲ����β���
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    // ��ʱ����������(�Ĳ�����)
    void tick() {
        // ��ʱ���ᰴ���趨�ĳ�ʱʱ�����δ�����Ӧ�Ļص�����,ͬʱд����־�����ڴ�������������Ƴ��ö�ʱ���ڵ㡣�������Ϊ�գ���ʱ������ִ���κβ�����
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