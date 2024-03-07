#include "../inc/HttpConnection.h"

int HttpConnection::UserCount = 0;
Mutex HttpConnection::mutex;

map<string, string> users; // 用户集合

// 网站根目录路径
const char *docRoot = "/home/nanhui/Project/Linux_WebServer/root";

// 定义HTTP响应的状态信息
const char *okTitle200 = "OK";
const char *errorTitle400 = "Bad Request";
const char *errorForm400 = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *errorTitle403 = "Forbidden";
const char *errorForm403 = "You do not have permission to get file form this server.\n";
const char *errorTitle404 = "Not Found";
const char *errorForm404 = "The requested file was not found on this server.\n";
const char *errorTitle500 = "Internal Error";
const char *errorForm500 = "There was an unusual problem serving the request file.\n";

struct sockaddr_in *HttpConnection::getAddress() {
    return &myAddress;
}

void HttpConnection::init(int socketfd, const struct sockaddr_in &addr) {
    mySocketfd = socketfd;          // 记录通信套接字
    myAddress = addr;               // 记录客户端地址
    addfd(epollfd, socketfd, true); // 往内核事件表中注册读事件，开启EPOLLONESHOT
    UserCount++;                    // 记录用户数量
    init();                         // 调用内部初始化函数
}

void HttpConnection::init() {
    connection = NULL;                      // 数据库连接初始化
    bytesToSend = 0;                        // 待发送字节数初始化
    bytesHaveSend = 0;                      // 已发送字节数初始化
    myCheckState = CHECK_STATE_REQUESTLINE; // 主状态机状态默认为解析请求行状态
    myLinger = false;                       // keep alive开关初始化
    myMethod = GET;                         // 请求方法初始化为GET请求
    myUrl = 0;                              // 请求地址初始化
    myVersion = 0;                          // 请求版本初始化
    myContentLength = 0;                    // 请求主体大小初始化
    myHost = 0;                             // 请求域名初始化
    myStartLine = 0;                        // 解析位置初始化
    myCheckedIndex = 0;                     // 已读位置初始化
    myReadIndex = 0;                        // 读位置初始化
    myWriteIndex = 0;                       // 写位置初始化
    cgi = false;                            // cgi开关初始化为false
    memset(myReadBuf, 0, readBufSize);      // 读缓冲区初始化
    memset(myWriteBuf, 0, writeBufSize);    // 写缓冲区初始化
    memset(myFileName, 0, fileNameLen);     // 文件名称缓冲区初始化
}

void HttpConnection::initMysqlUsers(SqlConnectionPool *sqlConnectionPool) {
    // 从数据库连接池中获取连接
    MYSQL *connection = NULL;
    SqlConnectionPoolRAII(&connection, sqlConnectionPool);

    // 从user表中查询用户名和密码
    string sql = "select username,passwd from user";
    if (mysql_query(connection, sql.c_str())) {
        LOG_ERROR("select error: %s.\n", mysql_error(connection));
    }

    // 获取查询后的结果集
    MYSQL_RES *result = mysql_store_result(connection);

    // 从结果集中获取一行，将对应的用户名和密码存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string username = row[0];
        string password = row[1];
        // cout << username << " " << password << endl;
        users[username] = password;
    }
}

bool HttpConnection::readOnce() {
    if (myReadIndex >= readBufSize) { // 读缓冲区满了
        return false;
    }
    int readBytes = 0;

#ifdef CONNECT_FD_ET // ET模式
    while (1) {      // 循环读取
        readBytes = recv(mySocketfd, myReadBuf + myReadIndex, readBufSize - myReadIndex, 0);
        if (readBytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return false;
        } else if (readBytes == 0) {
            return false;
        }
        myReadIndex += readBytes; // 更新读缓冲区下标位置
    }
    // printf("%s\n", myReadBuf);
    return true;
#endif

#ifdef CONNECT_FD_LT // LT模式
    readBytes = recv(mySocketfd, myReadBuf + myReadIndex, readBufSize - myReadIndex, 0);
    if (readBytes <= 0) {
        return false;
    }
    myReadIndex += readBytes;
    return true;
#endif
}

HttpConnection::LINE_STATE HttpConnection::parseLine() {
    // printf("parseLine success\n");
    char ch;
    // 查找\r\n字符，如果找到其中一个而找不到另一个就是LINE_BAD无法解析，如果两个都找到就是LINE_OK解析成功，如果都找不到就是LINE_OPEN报文不完整
    // printf("%d\n", myCheckedIndex);
    // printf("%d\n", myReadIndex);
    for (; myCheckedIndex < myReadIndex; ++myCheckedIndex) {
        ch = myReadBuf[myCheckedIndex];
        // printf("%c\n", ch);
        if (ch == '\r') {
            // printf("find r\n");
            if (myCheckedIndex + 1 == myReadIndex) {
                return LINE_OPEN;
            } else if (myReadBuf[myCheckedIndex + 1] == '\n') {
                // printf("ok\n");
                myReadBuf[myCheckedIndex++] = 0;
                myReadBuf[myCheckedIndex++] = 0;
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (ch == '\n') {
            if (myCheckedIndex > 0 && myReadBuf[myCheckedIndex - 1] == '\r') {
                myReadBuf[myCheckedIndex++] = 0;
                myReadBuf[myCheckedIndex++] = 0;
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HttpConnection::HTTP_CODE HttpConnection::parseRequestLine(char *text) {
    // 解析请求行，获取请求方法，目标url和http版本号,各个部分使用空格或制表符分隔
    // 该函数返回第一个查找到目标字符串任意字符的位置
    char *str = strpbrk(text, " \t");
    if (!str) {
        return BAD_REQUEST;
    }
    *str++ = 0;
    char *method = text; // 记录请求方法

    LOG_INFO("%s", method);
    Log::getInstance()->flush();

    // 不区分大小写的比较字符串是否相等，判断请求方法是GET还是POST
    if (strcasecmp(method, "GET") == 0) {
        myMethod = GET;
    } else if (strcasecmp(method, "POST") == 0) {
        myMethod = POST;
        cgi = true; // POST请求开启cgi验证
    } else {
        return BAD_REQUEST;
    }
    // 该函数返回最长的空格或制表符前缀的长度
    str += strspn(str, " \t");
    myUrl = str; // 记录URL
    str = strpbrk(str, " \t");
    if (!str) {
        return BAD_REQUEST;
    }
    *str++ = 0;
    str += strspn(str, " \t");
    myVersion = str; // 记录HTTP版本号

    LOG_INFO("%s", myVersion);
    Log::getInstance()->flush();

    // 判断版本号
    if (strcasecmp(myVersion, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }

    // 判断URL是否存在HTTP前缀
    if (strncasecmp(myUrl, "http://", 7) == 0) {
        myUrl += 7;
        myUrl = strchr(myUrl, '/');
    } else if (strncasecmp(myUrl, "https://", 8) == 0) {
        myUrl += 8;
        myUrl = strchr(myUrl, '/');
    }

    if (!myUrl || myUrl[0] != '/') {
        return BAD_REQUEST;
    }
    // 只有一个'/'，加上资源页面后缀
    if (strlen(myUrl) == 1) {
        strcat(myUrl, "judge.html");
    }

    LOG_INFO("%s", myUrl);
    Log::getInstance()->flush();

    myCheckState = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseHeader(char *text) {
    // 判断是空行还是请求头
    if (text[0] == '\0') {
        // printf("blank line\n");                 // 空行
        if (myContentLength != 0) {             // 判断是否是POST请求
            myCheckState = CHECK_STATE_CONTENT; // POST需要解析请求体
            return NO_REQUEST;
        }
        // GET请求直接返回成功
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) { // 解析请求头中连接字段
        text += 11;
        text += strcspn(text, " \t"); // 后移跳过所有的空格和制表符
        if (strcasecmp(text, "keep-alive") == 0) {
            myLinger = true; // 开启keep-alive
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) { // 解析请求头中内容长度字段
        text += 15;
        text += strcspn(text, " \t");
        myContentLength = atoi(text);                // 记录请求体长度
    } else if (strncasecmp(text, "Host:", 5) == 0) { // 解析请求头中HOST字段
        text += 5;
        text += strcspn(text, " \t");
        myHost = text; // 记录HOST
    } else {
        LOG_INFO("unknow header : %s.\n", text); // 未知请求头
        Log::getInstance()->flush();
    }
    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseContent(char *text) {
    // 判断请求体是否完整
    if (myReadIndex >= myContentLength + myCheckedIndex) {
        text[myContentLength] = 0; // 在最后加上结束标识符
        myString = text;           // 这里POST请求体存放的是用户名和密码
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

char *HttpConnection::getLine() {
    return myReadBuf + myStartLine; // 返回还未处理的字符的位置指针
}

HttpConnection::HTTP_CODE HttpConnection::processRead() {
    // printf("processRead success\n");
    LINE_STATE lineState = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    // 调用从机读取解析一行，判断是否能进入主机进行解析
    while ((myCheckState == CHECK_STATE_CONTENT && lineState == LINE_OK) || (lineState = parseLine()) == LINE_OK) {
        // printf("success\n");
        text = getLine();             // 从读缓冲区获取未处理字符首位置
        myStartLine = myCheckedIndex; // 更新已处理字符位置
        LOG_INFO("%s", text);         // 输出日志
        Log::getInstance()->flush();  // 刷新
        switch (myCheckState) {       // 判断主机状态
        case CHECK_STATE_REQUESTLINE: {
            ret = parseRequestLine(text); // 解析请求行
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER: {
            ret = parseHeader(text); // 解析请求头
            // printf("ret:%d\n", ret);
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            } else if (ret == GET_REQUEST) {
                return doRequest();
            }
            break;
        }
        case CHECK_STATE_CONTENT: {
            ret = parseContent(text); // 解析请求体
            if (ret == GET_REQUEST) {
                return doRequest();
            }
            lineState = LINE_OPEN;
            break;
        }
        default:
            return INTERNAL_ERROR; // 一般不会触发
        }
    }
    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::doRequest() {
    strcpy(myFileName, docRoot); // 获取网站根目录路径
    int len = strlen(docRoot);
    const char *p = strrchr(myUrl, '/'); // 获取最后一个'/'位置

    // 处理cgi，登录验证和注册验证
    if (cgi == true && (*(p + 1) == '2' || *(p + 1) == '3')) {
        char flag = myUrl[1];
        char *myRealUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(myRealUrl, "/");
        strcat(myRealUrl, myUrl + 2);
        strncpy(myFileName + len, myRealUrl, fileNameLen - len - 1);
        free(myRealUrl);

        // 提取用户名和密码
        string username, password;
        int i;
        for (i = 5; myString[i] != '&'; ++i) {
            username += myString[i];
        }
        int j = 0;
        for (i = i + 10; myString[i] != 0; ++i, ++j) {
            password += myString[i];
        }

        // 注册校验
        if (*(p + 1) == '3') {
            string sql = "insert into user(username,passwd) values('" + username + "','" + password + "')";
            // 判重
            HttpConnection::mutex.lock();
            if (users.find(username) == users.end()) {
                int ret = mysql_query(connection, sql.c_str());         // 插入数据库
                users.insert(pair<string, string>(username, password)); // 插入用户集合
                HttpConnection::mutex.unlock();

                if (!ret) {
                    strcpy(myUrl, "/login.html");
                } else {
                    strcpy(myUrl, "/registerError.html");
                }
            } else {
                HttpConnection::mutex.unlock();
                strcpy(myUrl, "/registerError.html");
            }
            // 登录校验
        } else if (*(p + 1) == '2') {
            if (users.find(username) != users.end() && users[username] == password) {
                strcpy(myUrl, "/welcome.html");
            } else {
                strcpy(myUrl, "/loginError.html");
            }
        }
    }

    // 添加对应的资源页面
    if (*(p + 1) == '0') {
        char *myRealUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(myRealUrl, "/register.html");
        strncpy(myFileName + len, myRealUrl, strlen(myRealUrl));
        free(myRealUrl);
    } else if (*(p + 1) == '1') {
        char *myRealUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(myRealUrl, "/login.html");
        strncpy(myFileName + len, myRealUrl, strlen(myRealUrl));
        free(myRealUrl);
    } else if (*(p + 1) == '5') {
        char *myRealUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(myRealUrl, "/picture.html");
        strncpy(myFileName + len, myRealUrl, strlen(myRealUrl));
        free(myRealUrl);
    } else if (*(p + 1) == '6') {
        char *myRealUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(myRealUrl, "/video.html");
        strncpy(myFileName + len, myRealUrl, strlen(myRealUrl));
        free(myRealUrl);
    } else if (*(p + 1) == '7') {
        char *myRealUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(myRealUrl, "/fans.html");
        strncpy(myFileName + len, myRealUrl, strlen(myRealUrl));
        free(myRealUrl);
    } else {
        strncpy(myFileName + len, myUrl, fileNameLen - len - 1);
    }
    // printf("%s\n", myFileName);

    if (stat(myFileName, &myFileStat) < 0) {
        return NO_RESOURCE;
    }

    // 没有读取权限
    if (!(myFileStat.st_mode & S_IROTH)) {
        return FORBIDDEN_REQUEST;
    }

    // 访问资源为目录
    if (S_ISDIR(myFileStat.st_mode)) {
        return BAD_REQUEST;
    }

    // 打开文件，映射到内存
    int fd = open(myFileName, O_RDONLY);
    myFileAddress = (char *)mmap(0, myFileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void HttpConnection::unmap() {
    if (myFileAddress) {
        munmap(myFileAddress, myFileStat.st_size);
        myFileAddress = 0;
    }
}

bool HttpConnection::addResponse(const char *format, ...) {
    if (myWriteIndex >= writeBufSize) { // 写入内容超出写缓冲区大小
        return false;
    }
    // 定义可变参数列表
    va_list valist;
    va_start(valist, format);
    // 将数据根据format的格式写入写缓冲区
    int len = vsnprintf(myWriteBuf + myWriteIndex, writeBufSize - myWriteIndex - 1, format, valist);
    if (len >= (writeBufSize - myWriteIndex - 1)) {
        va_end(valist);
        return false;
    }
    // 更新写缓冲区指针位置
    myWriteIndex += len;
    va_end(valist);
    LOG_INFO("request:%s.", myWriteBuf);
    Log::getInstance()->flush();
    return true;
}

bool HttpConnection::addStatusLine(int status, const char *title) {
    return addResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpConnection::addHeaders(int contentLength) {
    addContentLength(contentLength);
    addLinger();
    addBlankLine();
}

bool HttpConnection::addContentLength(int contentLength) {
    return addResponse("Content-Length:%d\r\n", contentLength);
}

bool HttpConnection::addContentType() {
    return addResponse("Content-Type:%s\r\n", "text/html");
}

bool HttpConnection::addLinger() {
    return addResponse("Connection:%s\r\n", (myLinger == true) ? "keep-alive" : "close");
}

bool HttpConnection ::addBlankLine() {
    return addResponse("%s", "\r\n");
}

bool HttpConnection::addContent(const char *content) {
    return addResponse("%s", content);
}

bool HttpConnection::processWrite(HTTP_CODE flag) {
    // printf("process write\n");
    switch (flag) {
    case INTERNAL_ERROR: {
        addStatusLine(500, errorTitle500);
        addHeaders(strlen(errorForm500));
        if (!addContent(errorForm500)) {
            return false;
        }
        break;
    }
    case FORBIDDEN_REQUEST: {
        addStatusLine(403, errorTitle403);
        addHeaders(strlen(errorForm403));
        if (!addContent(errorForm403)) {
            return false;
        }
        break;
    }
    case FILE_REQUEST: {
        addStatusLine(200, okTitle200);
        if (myFileStat.st_size != 0) {
            addHeaders(myFileStat.st_size);
            myIovec[0].iov_base = myWriteBuf;
            myIovec[0].iov_len = myWriteIndex;
            myIovec[1].iov_base = myFileAddress;
            myIovec[1].iov_len = myFileStat.st_size;
            myIovecCount = 2;
            bytesToSend = myWriteIndex + myFileStat.st_size;
            return true;
        } else {
            const char *okString = "<html><body></body></html>";
            addHeaders(strlen(okString));
            if (!addContent(okString)) {
                return false;
            }
            break;
        }
    }
    default:
        return false;
    }
    myIovec[0].iov_base = myWriteBuf;
    myIovec[0].iov_len = myWriteIndex;
    myIovecCount = 1;
    bytesToSend = myWriteIndex;
    return true;
}

bool HttpConnection::write() {
    int ret = 0;
    // 响应报文为空
    if (bytesToSend == 0) {
        modfd(epollfd, mySocketfd, EPOLLIN);
        init();
        return true;
    }
    while (1) {
        // 将响应报文发送出去
        ret = writev(mySocketfd, myIovec, myIovecCount);

        // 未发送成功
        if (ret < 0) {
            // 判断缓冲区是否满了
            if (errno == EAGAIN) {
                modfd(epollfd, mySocketfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        // 手动更新待发送的数据位置
        bytesHaveSend += ret;
        bytesToSend -= ret;
        if (bytesHaveSend >= myIovec[0].iov_len) {
            myIovec[0].iov_len = 0;
            myIovec[1].iov_base = myFileAddress + (bytesHaveSend - myWriteIndex);
            myIovec[1].iov_len = bytesToSend;
        } else {
            myIovec[0].iov_base = myWriteBuf + bytesHaveSend;
            myIovec[0].iov_len = myIovec[0].iov_len - bytesHaveSend;
        }

        // 数据发送完成
        if (bytesToSend <= 0) {
            unmap();
            modfd(epollfd, mySocketfd, EPOLLIN);
            // 判断是否为keep-alive
            if (myLinger) {
                init();
                return true;
            } else {
                return false;
            }
        }
    }
}

void HttpConnection::closeConnection(bool realClose) {
    if (realClose && mySocketfd != -1) {
        removefd(epollfd, mySocketfd);
        mySocketfd = -1;
        UserCount--;
    }
}

void HttpConnection::process() {
    // printf("process success\n");
    HTTP_CODE readRet = processRead();
    if (readRet == NO_REQUEST) {
        modfd(epollfd, mySocketfd, EPOLLIN);
        return;
    }
    // printf("%d\n", readRet);
    bool writeRet = processWrite(readRet);
    // cout << writeRet << endl;
    if (!writeRet) {
        closeConnection();
    }
    modfd(epollfd, mySocketfd, EPOLLOUT);
}