#ifndef __UTILS_H__
#define __UTILS_H__

#include "wrap.h"

#define TIMESLOT 15

class Utils {
public:
    static int *utilsPipefd; // pipefd
    static int utilsEpollfd; // epollfd

public:
    // 设置文件描述符为非阻塞
    int setNonBlocking(int fd) {
        int oldOption = fcntl(fd, F_GETFL);
        int newOption = oldOption | O_NONBLOCK;
        fcntl(fd, F_SETFL, newOption);
        return oldOption;
    }
    // 往epoll注册fd读事件，选择开启ET和EPOLLONESHOT事件
    void addfd(int epollfd, int fd, bool oneShot, bool isET) {
        epoll_event event;
        event.data.fd = fd;
        if (isET) {
            event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
        } else {
            event.events = EPOLLIN | EPOLLRDHUP;
        }
        if (oneShot) {
            event.events |= EPOLLONESHOT;
        }
        epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
        setNonBlocking(fd);
    }
    // 信号处理函数
    static void sigHandler(int sig) {
        int tempErrno = errno;
        send(utilsPipefd[1], (char *)&sig, 1, 0);
        errno = tempErrno;
    }
    // 设置信号处理函数
    void addSigHandler(int sig, void (*handler)(int), bool restart = true) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = handler;
        if (restart) {
            sa.sa_flags |= SA_RESTART;
        }
        sigfillset(&sa.sa_mask);
        assert(sigaction(sig, &sa, NULL) != -1);
    }
};
int pipefd[2];
int *Utils::utilsPipefd = pipefd;
int Utils::utilsEpollfd = 0;

#endif