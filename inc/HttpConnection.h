#ifndef __HTTPCONNECTION_H__
#define __HTTPCONNECTION_H__

#include "Locker.h"
#include "Log.h"
#include "SqlConnectionPool.h"
#include "Uilts.h"
#include "wrap.h"

class HttpConnection {
public:
    static const int fileNameLen = 200;   // ���ö�ȡ�ļ������ƴ�С
    static const int readBufSize = 2048;  // ���ö���������С
    static const int writeBufSize = 1024; // ����д��������С

    MYSQL *connection; // ���ݿ�����
    static Mutex mutex;
    static int UserCount; // ��¼��������

    // HTTP���ĵ����󷽷�
    enum METHOD {
        GET = 0, // GET����
        POST,    // POST����
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    // ���Ľ������
    enum HTTP_CODE {
        NO_REQUEST,        // ������������Ҫ������ȡ����������
        GET_REQUEST,       // �����������HTTP����
        NO_RESOURCE,       // ������Դ������
        BAD_REQUEST,       // HTTP���������﷨�����������ԴΪĿ¼
        FORBIDDEN_REQUEST, // ������Դ��ֹ���ʣ�û�ж�ȡȨ��
        FILE_REQUEST,      // ������Դ������������
        INTERNAL_ERROR,    // �������ڲ�����
        CLOSED_CONNECTION  // �ر�����

    };
    // ��״̬����״̬
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0, // ����������
        CHECK_STATE_HEADER,          // ��������ͷ
        CHECK_STATE_CONTENT          // ����������
    };
    // ��״̬��״̬
    enum LINE_STATE {
        LINE_OK = 0, // ������ȡһ��
        LINE_BAD,    // �����﷨����
        LINE_OPEN    // ��ȡ���в�����
    };

public:
    HttpConnection(){};
    ~HttpConnection(){};
    void init(int socketfd, const struct sockaddr_in &addr); // ��ʼ������
    void closeConnection(bool realClose = true);
    void process();
    bool readOnce(); // ��ȡ���ݷ����������
    bool write();
    struct sockaddr_in *getAddress();
    static void initMysqlUsers(SqlConnectionPool *sqlConnectionPool); // ��ʼ�����ݿ��û������û����ݴ����ݿ��û�����ȡ��

private:
    int mySocketfd;               // ͨ���׽���
    struct sockaddr_in myAddress; // �ͻ��˵�ַ

    char myReadBuf[readBufSize];   // ��������
    int myReadIndex;               // �����������һ���ֽ�λ��
    int myCheckedIndex;            // ���������Ѿ���ȡ��λ��
    int myStartLine;               // ���������Ѿ��������ַ�����
    char myWriteBuf[writeBufSize]; // д������
    int myWriteIndex;              // д���������һ���ֽ�λ��

    CHECK_STATE myCheckState; // ��״̬��״̬
    METHOD myMethod;          // ���󷽷�

    char myFileName[fileNameLen]; //
    char *myUrl;                  // �������е�url��ַ
    char *myVersion;              // �������еİ汾��
    char *myHost;                 // ����ͷ�е�Host�ֶ�
    int myContentLength;          // ����ͷ��Content-length�ֶΣ������峤��
    bool myLinger;                // keep-alive����
    char *myFileAddress;          // mmap���ص�ַ
    struct stat myFileStat;       //
    struct iovec myIovec[2];      //
    int myIovecCount;             //
    bool cgi;                     // cgi��֤����
    char *myString;               // �洢����ͷ����
    int bytesToSend;              // �������ֽ�
    int bytesHaveSend;            // �ѷ����ֽ�

private:
    void init(); // �ڲ���ʼ��

    HTTP_CODE processRead();           // �Ӷ��������ж�ȡ������������(��״̬��)
    bool processWrite(HTTP_CODE flag); // ��д������д����Ӧ����

    HTTP_CODE parseRequestLine(char *text); // ��״̬������������
    HTTP_CODE parseHeader(char *text);      // ��״̬����������ͷ
    HTTP_CODE parseContent(char *text);     // ��״̬������������

    LINE_STATE parseLine(); // ��״̬�������ڶ�ȡ����\r\n��β��һ������

    char *getLine(); // ��ָ������ƫ�ƣ�ָ��δ������ַ�

    HTTP_CODE doRequest();                             // ������Ӧ����
    bool addResponse(const char *format, ...);         // ��д������д����Ӧ���ģ�ʹ�ÿɱ������ǿ������
    bool addContent(const char *content);              // �����Ӧ����
    bool addStatusLine(int status, const char *title); // ���״̬��
    bool addHeaders(int contentLength);                // �����Ϣ��ͷ
    bool addContentType();                             // ���Content-Type�ֶ�
    bool addContentLength(int contentLength);          // ���Content-Length�ֶ�
    bool addLinger();                                  // ���Connection�ֶ�
    bool addBlankLine();                               // ��ӿ���

    void unmap(); // �ͷ�mmap�����ĵ�ַ
};

#endif