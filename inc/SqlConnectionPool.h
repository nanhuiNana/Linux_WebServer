#ifndef __SQLCONNECTIONPOOL_H__
#define __SQLCONNECTIONPOOL_H__

#include "Locker.h"
#include "wrap.h"

class SqlConnectionPool {
private:
    unsigned int myMaxConnection;       // 最大连接数
    unsigned int myAvailableConnection; // 可用连接数
    unsigned int myUsedConnection;      // 已用连接数

    Mutex myMutex;                  // 互斥锁
    Sem mySem;                      // 信号量
    list<MYSQL *> myConnectionList; // 连接池

    string myUrl;          // 主机地址
    string myPort;         // 数据库端口号
    string myUsername;     // 登录数据库用户名
    string myPassword;     // 登录数据库密码
    string myDatabaseName; // 使用的数据库名

private:
    SqlConnectionPool();
    ~SqlConnectionPool();

public:
    // 懒汉单例模式
    static SqlConnectionPool *getInstance();

    // 初始化
    void init(string url, int port, string username, string password, string databaseName, unsigned int maxConnection);

    // 获取连接
    MYSQL *getConnection();
    // 释放连接
    bool releaseConnection(MYSQL *connection);
    // 获取空闲连接数
    int getAvailableConnection();
    // 销毁所有连接
    void destroyPool();
};

class SqlConnectionPoolRAII {
private:
    MYSQL *connection;
    SqlConnectionPool *sqlConnectionPool;

public:
    SqlConnectionPoolRAII(MYSQL **connection, SqlConnectionPool *sqlConnectionPool);
    ~SqlConnectionPoolRAII();
};

#endif