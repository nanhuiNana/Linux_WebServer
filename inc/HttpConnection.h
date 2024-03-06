#ifndef __HTTPCONNECTION_H__
#define __HTTPCONNECTION_H__

#include "Locker.h"
#include "Log.h"
#include "SqlConnectionPool.h"
#include "Uilts.h"
#include "wrap.h"

class HttpConnection {
public:
    static const int fileNameLen = 200;   // 设置读取文件的名称大小
    static const int readBufSize = 2048;  // 设置读缓冲区大小
    static const int writeBufSize = 1024; // 设置写缓冲区大小

    MYSQL *connection; // 数据库连接
    static Mutex mutex;
    static int UserCount; // 记录连接数量

    // HTTP报文的请求方法
    enum METHOD {
        GET = 0, // GET请求
        POST,    // POST请求
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    // 报文解析结果
    enum HTTP_CODE {
        NO_REQUEST,        // 请求不完整，需要继续读取请求报文数据
        GET_REQUEST,       // 获得了完整的HTTP请求
        NO_RESOURCE,       // 请求资源不存在
        BAD_REQUEST,       // HTTP请求报文有语法错误或请求资源为目录
        FORBIDDEN_REQUEST, // 请求资源禁止访问，没有读取权限
        FILE_REQUEST,      // 请求资源可以正常访问
        INTERNAL_ERROR,    // 服务器内部错误
        CLOSED_CONNECTION  // 关闭连接

    };
    // 主状态机的状态
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0, // 解析请求行
        CHECK_STATE_HEADER,          // 解析请求头
        CHECK_STATE_CONTENT          // 解析请求体
    };
    // 从状态机状态
    enum LINE_STATE {
        LINE_OK = 0, // 完整读取一行
        LINE_BAD,    // 报文语法有误
        LINE_OPEN    // 读取的行不完整
    };

public:
    HttpConnection(){};
    ~HttpConnection(){};
    void init(int socketfd, const struct sockaddr_in &addr); // 初始化连接
    void closeConnection(bool realClose = true);
    void process();
    bool readOnce(); // 读取数据放入读缓冲区
    bool write();
    struct sockaddr_in *getAddress();
    static void initMysqlUsers(SqlConnectionPool *sqlConnectionPool); // 初始化数据库用户表，将用户数据从数据库用户表中取出

private:
    int mySocketfd;               // 通信套接字
    struct sockaddr_in myAddress; // 客户端地址

    char myReadBuf[readBufSize];   // 读缓冲区
    int myReadIndex;               // 读缓冲区最后一个字节位置
    int myCheckedIndex;            // 读缓冲区已经读取的位置
    int myStartLine;               // 读缓冲区已经解析的字符个数
    char myWriteBuf[writeBufSize]; // 写缓冲区
    int myWriteIndex;              // 写缓冲区最后一个字节位置

    CHECK_STATE myCheckState; // 主状态机状态
    METHOD myMethod;          // 请求方法

    char myFileName[fileNameLen]; //
    char *myUrl;                  // 请求行中的url地址
    char *myVersion;              // 请求行中的版本号
    char *myHost;                 // 请求头中的Host字段
    int myContentLength;          // 请求头中Content-length字段，请求体长度
    bool myLinger;                // keep-alive开关
    char *myFileAddress;          // mmap返回地址
    struct stat myFileStat;       //
    struct iovec myIovec[2];      //
    int myIovecCount;             //
    bool cgi;                     // cgi验证开关
    char *myString;               // 存储请求头数据
    int bytesToSend;              // 待发送字节
    int bytesHaveSend;            // 已发送字节

private:
    void init(); // 内部初始化

    HTTP_CODE processRead();           // 从读缓冲区中读取并处理请求报文(主状态机)
    bool processWrite(HTTP_CODE flag); // 向写缓冲区写入响应报文

    HTTP_CODE parseRequestLine(char *text); // 主状态机解析请求行
    HTTP_CODE parseHeader(char *text);      // 主状态机解析请求头
    HTTP_CODE parseContent(char *text);     // 主状态机解析请求体

    LINE_STATE parseLine(); // 从状态机，用于读取出以\r\n结尾的一行内容

    char *getLine(); // 将指针往后偏移，指向未处理的字符

    HTTP_CODE doRequest();                             // 生成响应报文
    bool addResponse(const char *format, ...);         // 往写缓冲区写入响应报文，使用可变参数增强复用性
    bool addContent(const char *content);              // 添加响应正文
    bool addStatusLine(int status, const char *title); // 添加状态行
    bool addHeaders(int contentLength);                // 添加消息报头
    bool addContentType();                             // 添加Content-Type字段
    bool addContentLength(int contentLength);          // 添加Content-Length字段
    bool addLinger();                                  // 添加Connection字段
    bool addBlankLine();                               // 添加空行

    void unmap(); // 释放mmap出来的地址
};

#endif