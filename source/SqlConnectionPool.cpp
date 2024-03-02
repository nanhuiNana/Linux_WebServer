#include "../inc/SqlConnectionPool.h"

SqlConnectionPool::SqlConnectionPool() {
    myAvailableConnection = 0;
    myUsedConnection = 0;
}

SqlConnectionPool::~SqlConnectionPool() {
    destroyPool();
}

void SqlConnectionPool::init(string url, int port, string username, string password, string databaseName, unsigned int maxConnection) {
    // ��������Ա��ֵ
    myUrl = url;
    myPort = port;
    myUsername = username;
    myPassword = password;
    myDatabaseName = databaseName;
    myMutex.lock(); // ����
    // �������ݿ����ӣ��������ӳ���
    for (int i = 0; i < maxConnection; i++) {
        MYSQL *connection = NULL;
        connection = Mysql_init(connection);
        connection = Mysql_real_connect(connection, url.c_str(), username.c_str(), password.c_str(), databaseName.c_str(), port, nullptr, 0);
        myConnectionList.push_back(connection);
        ++myAvailableConnection; // ���¿�����������
    }
    // �ź��������ڶ����ݿ����ӳؽ���ͬ������
    mySem = Sem(myAvailableConnection);
    // ��ʼ�����ֵ
    maxConnection = myAvailableConnection;
    cout << "ok: " << maxConnection << endl;
    myMutex.unlock(); // ����
}

void SqlConnectionPool::destroyPool() {
    myMutex.lock();
    if (myConnectionList.size() > 0) {
        for (auto connection : myConnectionList) {
            mysql_close(connection); // �ر�����
        }
        myAvailableConnection = 0;
        myUsedConnection = 0;
        myConnectionList.clear();
    }
    myMutex.unlock();
}

MYSQL *SqlConnectionPool::getConnection() {
    MYSQL *connection = NULL;
    if (myConnectionList.empty()) { // �ж����ӳ��Ƿ�Ϊ��
        return NULL;
    }
    mySem.wait();                          // �߳������ȴ�
    myMutex.lock();                        // ����
    connection = myConnectionList.front(); // ��ȡ����
    myConnectionList.pop_front();          // �����ӵ���
    --myAvailableConnection;               // ���¿���������
    ++myUsedConnection;                    // ��������������
    myMutex.unlock();                      // ����
    return connection;
}

bool SqlConnectionPool::releaseConnection(MYSQL *connection) {
    if (connection == NULL) { // �ж������Ƿ�Ϊ��
        return false;
    }
    myMutex.lock();                         // ����
    myConnectionList.push_back(connection); // �����ӹ黹
    ++myAvailableConnection;                // ���¿���������
    --myUsedConnection;                     // ��������������
    myMutex.unlock();                       // ����
    mySem.post();                           // ���������߳�
    return true;
}

int SqlConnectionPool::getAvailableConnection() {
    return myAvailableConnection;
}

SqlConnectionPool *SqlConnectionPool::getInstance() {
    static SqlConnectionPool sqlConnectionPool; // �ֲ���̬����
    return &sqlConnectionPool;
}

SqlConnectionPoolRAII::SqlConnectionPoolRAII(MYSQL **connection, SqlConnectionPool *sqlConnectionPool) {
    *connection = sqlConnectionPool->getConnection(); // ��ȡ����
    cout << "get success" << endl;
    // �Գ�Ա��ֵ
    this->connection = *connection;
    this->sqlConnectionPool = sqlConnectionPool;
}
SqlConnectionPoolRAII::~SqlConnectionPoolRAII() {
    cout << "release success" << endl;
    sqlConnectionPool->releaseConnection(connection); // �ͷ�����
}