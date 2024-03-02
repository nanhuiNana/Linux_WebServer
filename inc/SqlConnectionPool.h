#ifndef __SQLCONNECTIONPOOL_H__
#define __SQLCONNECTIONPOOL_H__

#include "Locker.h"
#include "wrap.h"

class SqlConnectionPool {
private:
    unsigned int myMaxConnection;       // ���������
    unsigned int myAvailableConnection; // ����������
    unsigned int myUsedConnection;      // ����������

    Mutex myMutex;                  // ������
    Sem mySem;                      // �ź���
    list<MYSQL *> myConnectionList; // ���ӳ�

    string myUrl;          // ������ַ
    string myPort;         // ���ݿ�˿ں�
    string myUsername;     // ��¼���ݿ��û���
    string myPassword;     // ��¼���ݿ�����
    string myDatabaseName; // ʹ�õ����ݿ���

private:
    SqlConnectionPool();
    ~SqlConnectionPool();

public:
    // ��������ģʽ
    static SqlConnectionPool *getInstance();

    // ��ʼ��
    void init(string url, int port, string username, string password, string databaseName, unsigned int maxConnection);

    // ��ȡ����
    MYSQL *getConnection();
    // �ͷ�����
    bool releaseConnection(MYSQL *connection);
    // ��ȡ����������
    int getAvailableConnection();
    // ������������
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