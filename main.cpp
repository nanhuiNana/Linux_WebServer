#include "inc/Uilts.h"

int main(int argc, char *argv[]) {
#ifdef ASYN_LOG
    // 异步日志
    Log::getIntance()->init("ServerLog", 2000, 800000, 8);
#endif

#ifdef SYN_LOG
    // 同步日志
    Log::getInstance()->init("ServerLog", 2000, 800000, 0);
#endif

    if (argc <= 1) { // 需要在参数中设置端口号
        printf("args is less than two\n");
        return -1;
    }

    int port = atoi(argv[1]); // 记录端口号

    //
    addSigHandler(SIGPIPE, SIG_IGN);

    // 创建数据库连接池
    SqlConnectionPool *conn_pool = SqlConnectionPool::getInstance();
    // 初始化数据库连接
    conn_pool->init("localhost", 3306, "root", "123456", "yourdb", 8);

    // 创建线程池
    PthreadPool<HttpConnection> *t_pool = NULL;
    try {
        t_pool = new PthreadPool<HttpConnection>(conn_pool);
    } catch (...) {
        return -1;
    }

    // 创建客户端数组（HTTP）
    HttpConnection *clients = new HttpConnection[MAX_FD];
    assert(clients);

    // 初始化数据库读取表
    clients->initMysqlUsers(conn_pool);

    // 创建监听套接字
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 设置服务器地址
    int ret = 0;
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 设置端口复用
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    // 绑定服务器地址
    ret = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    assert(ret >= 0);
    ret = listen(listenfd, 5);
    assert(ret >= 0);

    // 创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);

    // 添加监听描述符到内核事件表
    addfd(epollfd, listenfd, false);

    // 创建管道
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setNonBlocking(pipefd[1]);
    addfd(epollfd, pipefd[0], false);
    // printf("%d\n", pipefd[0]);

    // 设置信号捕捉
    addSigHandler(SIGALRM, sigHandler, false);
    addSigHandler(SIGTERM, sigHandler, false);

    // 创建客户端连接数据数组(定时器)
    ClientData *clientDatas = new ClientData[MAX_FD];

    bool timeout = false;
    alarm(TIMESLOT);

    bool serverStop = false;
    // printf("%d\n", listenfd);
    while (!serverStop) {
        int n = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        // printf("epoll wait success\n");
        if (n < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll failure.");
            Log::getInstance()->flush();
            break;
        }
        for (int i = 0; i < n; i++) {
            int socketfd = events[i].data.fd;

            // 处理新连接
            printf("%d\n", socketfd);
            // if (events[i].events & EPOLLIN)
            //     printf("true\n");
            if (socketfd == listenfd) {
                // printf("ok\n");

                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
#ifdef LISTEN_FD_LT
                int connectfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
                // printf("listen success\n");
                if (connectfd < 0) {
                    LOG_ERROR("%s : errno is %d.", "accept error", errno);
                    Log::getInstance()->flush();
                    continue;
                }
                if (HttpConnection::UserCount >= MAX_FD) {
                    LOG_ERROR("%s", "Internal server busy.");
                    Log::getInstance()->flush();
                    continue;
                }
                clients[connectfd].init(connectfd, client_addr);

                clientDatas[connectfd].clientAddr = client_addr;
                clientDatas[connectfd].socketfd = connectfd;
                Timer *timer = new Timer();
                timer->userData = &clientDatas[connectfd];
                timer->callbackFun = callbackFun;
                time_t now = time(NULL);
                timer->overtime = now + 3 * TIMESLOT;
                clientDatas[connectfd].timer = timer;
                timerSortList.addTimer(timer);
#endif

#ifdef LISTEN_FD_ET
                while (1) {
                    int connectfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
                    // printf("listen success\n");
                    if (connectfd < 0) {
                        LOG_ERROR("%s : errno is %d.", "accept error", errno);
                        Log::getInstance()->flush();
                        break;
                    }
                    if (HttpConnection::UserCount >= MAX_FD) {
                        LOG_ERROR("%s", "Internal server busy.");
                        Log::getInstance()->flush();
                        break;
                    }
                    clients[connectfd].init(connectfd, client_addr);

                    clientDatas[connectfd].clientAddr = client_addr;
                    clientDatas[connectfd].socketfd = connectfd;
                    Timer *timer = new Timer();
                    timer->userData = &clientDatas[connectfd];
                    timer->callbackFun = callbackFun;
                    time_t now = time(NULL);
                    timer->overtime = now + 3 * TIMESLOT;
                    clientDatas[connectfd].timer = timer;
                    timerSortList.addTimer(timer);
                }
                continue;
#endif
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                Timer *timer = clientDatas[socketfd].timer;
                timer->callbackFun(&clientDatas[socketfd]); // 关闭连接
                if (timer) {
                    timerSortList.deleteTimer(timer); // 移除定时器
                }
            } else if ((socketfd == pipefd[0]) && (events[i].events & EPOLLIN)) { // 处理信号
                // printf("pipe success\n");
                char signals[1024];
                ret = recv(socketfd, signals, sizeof(signals), 0);
                if (ret == -1 || ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; i++) {
                        switch (signals[i]) {
                        case SIGALRM:
                            timeout = true;
                            break;
                        case SIGTERM:
                            serverStop = true;
                            break;
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                // printf("connect success\n");
                Timer *timer = clientDatas[socketfd].timer;
                if (clients[socketfd].readOnce()) {
                    LOG_INFO("deal with the client(%s)", inet_ntoa(clients[socketfd].getAddress()->sin_addr));
                    // printf("read success\n");
                    t_pool->append(clients + socketfd);

                    if (timer) {
                        time_t now = time(NULL);
                        timer->overtime = now + 3 * TIMESLOT;
                        LOG_INFO("%s", "adjust timer once");
                        Log::getInstance()->flush();
                        timerSortList.adjustTimer(timer);
                    }
                } else {
                    timer->callbackFun(&clientDatas[socketfd]); // 关闭连接
                    if (timer) {
                        timerSortList.deleteTimer(timer); // 移除定时器
                    }
                }
            } else if (events[i].events & EPOLLOUT) {
                printf("write \n");
                Timer *timer = clientDatas[socketfd].timer;
                if (clients[socketfd].write()) {

                    if (timer) {
                        time_t now = time(NULL);
                        timer->overtime = now + 3 * TIMESLOT;
                        timerSortList.adjustTimer(timer);
                    }
                } else {
                    timer->callbackFun(&clientDatas[socketfd]); // 关闭连接
                    if (timer) {
                        timerSortList.deleteTimer(timer); // 移除定时器
                    }
                }
            }
        }
        if (timeout) {
            timerHandler();
            timeout = false;
        }
    }
    close(epollfd);
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    delete[] clients;
    delete[] clientDatas;
    delete t_pool;
    return 0;
}
