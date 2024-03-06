#ifndef __UTILS_H__
#define __UTILS_H__

#include "HttpConnection.h"
#include "Locker.h"
#include "Log.h"
#include "PthreadPool.h"
#include "SqlConnectionPool.h"
#include "Timer.h"
#include "wrap.h"

#define CONNECT_FD_ET // ͨ���׽���ETģʽ
// #define CONNECT_FD_LT // LTģʽ

#define LISTEN_FD_ET // �����׽���ETģʽ
// #define LISTEN_FD_LT // LTģʽ

#define SYN_LOG // ͬ��д��־
// #define ASYN_LOG // �첽д��־

class TimerSortList;
extern TimerSortList timeSortList;
const int MAX_FD = 65536;           // ����ļ�������
const int MAX_EVENT_NUMBER = 10000; // ����¼���
const int TIMESLOT = 5;             // ��С��ʱ��λ

extern int pipefd[2];
extern int epollfd;

// �����ļ�������Ϊ������
extern int setNonBlocking(int fd);

// ���ں��¼�����ע��fd���¼���ѡ����ET��EPOLLONESHOT�¼�
extern void addfd(int epollfd, int fd, bool oneShot);

// ���ں��¼�����ɾ��fd
extern void removefd(int epollfd, int fd);
// ���¼�����ΪEPOLLONESHOT
extern void modfd(int epollfd, int fd, int events);

// �źŴ�����
extern void sigHandler(int sig);
// �����źŴ�����
extern void addSigHandler(int sig, void (*handler)(int), bool restart = true);

extern void timerHandler();

struct ClientData;
extern void callbackFun(ClientData *userData);

#endif