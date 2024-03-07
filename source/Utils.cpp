#include "../inc/Uilts.h"

int pipefd[2];
TimerSortList timerSortList;
int epollfd = 0;

// 设置文件描述符为非阻塞
int setNonBlocking(int fd) {
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

// 往内核事件表中注册fd读事件，选择开启ET和EPOLLONESHOT事件
void addfd(int epollfd, int fd, bool oneShot) {
    epoll_event event;
    event.data.fd = fd;

#ifdef CONNECT_FD_ET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef CONNECT_FD_LT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

#ifdef LISTEN_FD_ET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef LISTEN_FD_LT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

    if (oneShot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

// 从内核事件表中删除fd
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

// 将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int events) {
    epoll_event event;
    event.data.fd = fd;

#ifdef CONNECT_FD_ET
    event.events = events | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
#endif

#ifdef CONNECT_FD_LT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 信号处理函数
void sigHandler(int sig) {
    int tempErrno = errno;
    send(pipefd[1], (char *)&sig, 1, 0);
    errno = tempErrno;
}

// 设置信号处理函数
void addSigHandler(int sig, void (*handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

// 定时处理函数，内部重置定时时间
void timerHandler() {
    timerSortList.tick();
    alarm(TIMESLOT);
}

// 定时器回调函数
void callbackFun(ClientData *userData) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, userData->socketfd, 0);
    assert(userData);
    close(userData->socketfd);
    HttpConnection::UserCount--;
    LOG_INFO("close fd %d", userData->socketfd);
    Log::getInstance()->flush();
}