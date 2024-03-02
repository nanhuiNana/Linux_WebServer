#include "../inc/SqlConnectionPool.h"

SqlConnectionPool::SqlConnectionPool() {
    myAvailableConnection = 0;
    myUsedConnection = 0;
}

SqlConnectionPool::~SqlConnectionPool() {
    destroyPool();
}

void SqlConnectionPool::init(string url, int port, string username, string password, string databaseName, unsigned int maxConnection) {
    // 给各个成员赋值
    myUrl = url;
    myPort = port;
    myUsername = username;
    myPassword = password;
    myDatabaseName = databaseName;
    myMutex.lock(); // 加锁
    // 创建数据库连接，加入连接池中
    for (int i = 0; i < maxConnection; i++) {
        MYSQL *connection = NULL;
        connection = Mysql_init(connection);
        connection = Mysql_real_connect(connection, url.c_str(), username.c_str(), password.c_str(), databaseName.c_str(), port, nullptr, 0);
        myConnectionList.push_back(connection);
        ++myAvailableConnection; // 更新可用连接数量
    }
    // 信号量，用于对数据库连接池进行同步操作
    mySem = Sem(myAvailableConnection);
    // 初始化最大值
    maxConnection = myAvailableConnection;
    cout << "ok: " << maxConnection << endl;
    myMutex.unlock(); // 解锁
}

void SqlConnectionPool::destroyPool() {
    myMutex.lock();
    if (myConnectionList.size() > 0) {
        for (auto connection : myConnectionList) {
            mysql_close(connection); // 关闭连接
        }
        myAvailableConnection = 0;
        myUsedConnection = 0;
        myConnectionList.clear();
    }
    myMutex.unlock();
}

MYSQL *SqlConnectionPool::getConnection() {
    MYSQL *connection = NULL;
    if (myConnectionList.empty()) { // 判断连接池是否为空
        return NULL;
    }
    mySem.wait();                          // 线程阻塞等待
    myMutex.lock();                        // 加锁
    connection = myConnectionList.front(); // 获取连接
    myConnectionList.pop_front();          // 将连接弹出
    --myAvailableConnection;               // 更新可用连接数
    ++myUsedConnection;                    // 更新已用连接数
    myMutex.unlock();                      // 解锁
    return connection;
}

bool SqlConnectionPool::releaseConnection(MYSQL *connection) {
    if (connection == NULL) { // 判断连接是否为空
        return false;
    }
    myMutex.lock();                         // 加锁
    myConnectionList.push_back(connection); // 将连接归还
    ++myAvailableConnection;                // 更新可用连接数
    --myUsedConnection;                     // 更新已用连接数
    myMutex.unlock();                       // 解锁
    mySem.post();                           // 唤醒阻塞线程
    return true;
}

int SqlConnectionPool::getAvailableConnection() {
    return myAvailableConnection;
}

SqlConnectionPool *SqlConnectionPool::getInstance() {
    static SqlConnectionPool sqlConnectionPool; // 局部静态变量
    return &sqlConnectionPool;
}

SqlConnectionPoolRAII::SqlConnectionPoolRAII(MYSQL **connection, SqlConnectionPool *sqlConnectionPool) {
    *connection = sqlConnectionPool->getConnection(); // 获取连接
    cout << "get success" << endl;
    // 对成员赋值
    this->connection = *connection;
    this->sqlConnectionPool = sqlConnectionPool;
}
SqlConnectionPoolRAII::~SqlConnectionPoolRAII() {
    cout << "release success" << endl;
    sqlConnectionPool->releaseConnection(connection); // 释放连接
}