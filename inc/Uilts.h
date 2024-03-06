#ifndef __UTILS_H__
#define __UTILS_H__

#include "HttpConnection.h"
#include "Locker.h"
#include "Log.h"
#include "PthreadPool.h"
#include "SqlConnectionPool.h"
#include "Timer.h"
#include "wrap.h"

#define CONNECT_FD_ET // 通信套接字ET模式
// #define CONNECT_FD_LT // LT模式

#define LISTEN_FD_ET // 监听套接字ET模式
// #define LISTEN_FD_LT // LT模式

#define SYN_LOG // 同步写日志
// #define ASYN_LOG // 异步写日志

class TimerSortList;
extern TimerSortList timeSortList;
const int MAX_FD = 65536;           // 最大文件描述符
const int MAX_EVENT_NUMBER = 10000; // 最大事件数
const int TIMESLOT = 5;             // 最小超时单位

extern int pipefd[2];
extern int epollfd;

// 设置文件描述符为非阻塞
extern int setNonBlocking(int fd);

// 往内核事件表中注册fd读事件，选择开启ET和EPOLLONESHOT事件
extern void addfd(int epollfd, int fd, bool oneShot);

// 从内核事件表中删除fd
extern void removefd(int epollfd, int fd);
// 将事件重置为EPOLLONESHOT
extern void modfd(int epollfd, int fd, int events);

// 信号处理函数
extern void sigHandler(int sig);
// 设置信号处理函数
extern void addSigHandler(int sig, void (*handler)(int), bool restart = true);

extern void timerHandler();

struct ClientData;
extern void callbackFun(ClientData *userData);

#endif