#include "../inc/HttpConnection.h"

int HttpConnection::UserCount = 0;
Mutex HttpConnection::mutex;

map<string, string> users; // �û�����

// ��վ��Ŀ¼·��
const char *docRoot = "/home/nanhui/Project/Linux_WebServer/root";

// ����HTTP��Ӧ��״̬��Ϣ
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
    mySocketfd = socketfd;          // ��¼ͨ���׽���
    myAddress = addr;               // ��¼�ͻ��˵�ַ
    addfd(epollfd, socketfd, true); // ���ں��¼�����ע����¼�������EPOLLONESHOT
    UserCount++;                    // ��¼�û�����
    init();                         // �����ڲ���ʼ������
}

void HttpConnection::init() {
    connection = NULL;                      // ���ݿ����ӳ�ʼ��
    bytesToSend = 0;                        // �������ֽ�����ʼ��
    bytesHaveSend = 0;                      // �ѷ����ֽ�����ʼ��
    myCheckState = CHECK_STATE_REQUESTLINE; // ��״̬��״̬Ĭ��Ϊ����������״̬
    myLinger = false;                       // keep alive���س�ʼ��
    myMethod = GET;                         // ���󷽷���ʼ��ΪGET����
    myUrl = 0;                              // �����ַ��ʼ��
    myVersion = 0;                          // ����汾��ʼ��
    myContentLength = 0;                    // ���������С��ʼ��
    myHost = 0;                             // ����������ʼ��
    myStartLine = 0;                        // ����λ�ó�ʼ��
    myCheckedIndex = 0;                     // �Ѷ�λ�ó�ʼ��
    myReadIndex = 0;                        // ��λ�ó�ʼ��
    myWriteIndex = 0;                       // дλ�ó�ʼ��
    cgi = false;                            // cgi���س�ʼ��Ϊfalse
    memset(myReadBuf, 0, readBufSize);      // ����������ʼ��
    memset(myWriteBuf, 0, writeBufSize);    // д��������ʼ��
    memset(myFileName, 0, fileNameLen);     // �ļ����ƻ�������ʼ��
}

void HttpConnection::initMysqlUsers(SqlConnectionPool *sqlConnectionPool) {
    // �����ݿ����ӳ��л�ȡ����
    MYSQL *connection = NULL;
    SqlConnectionPoolRAII(&connection, sqlConnectionPool);

    // ��user���в�ѯ�û���������
    string sql = "select username,passwd from user";
    if (mysql_query(connection, sql.c_str())) {
        LOG_ERROR("select error: %s.\n", mysql_error(connection));
    }

    // ��ȡ��ѯ��Ľ����
    MYSQL_RES *result = mysql_store_result(connection);

    // �ӽ�����л�ȡһ�У�����Ӧ���û������������map��
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string username = row[0];
        string password = row[1];
        // cout << username << " " << password << endl;
        users[username] = password;
    }
}

bool HttpConnection::readOnce() {
    if (myReadIndex >= readBufSize) { // ������������
        return false;
    }
    int readBytes = 0;

#ifdef CONNECT_FD_ET // ETģʽ
    while (1) {      // ѭ����ȡ
        readBytes = recv(mySocketfd, myReadBuf + myReadIndex, readBufSize - myReadIndex, 0);
        if (readBytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return false;
        } else if (readBytes == 0) {
            return false;
        }
        myReadIndex += readBytes; // ���¶��������±�λ��
    }
    // printf("%s\n", myReadBuf);
    return true;
#endif

#ifdef CONNECT_FD_LT // LTģʽ
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
    // ����\r\n�ַ�������ҵ�����һ�����Ҳ�����һ������LINE_BAD�޷�����������������ҵ�����LINE_OK�����ɹ���������Ҳ�������LINE_OPEN���Ĳ�����
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
    // ���������У���ȡ���󷽷���Ŀ��url��http�汾��,��������ʹ�ÿո���Ʊ���ָ�
    // �ú������ص�һ�����ҵ�Ŀ���ַ��������ַ���λ��
    char *str = strpbrk(text, " \t");
    if (!str) {
        return BAD_REQUEST;
    }
    *str++ = 0;
    char *method = text; // ��¼���󷽷�

    LOG_INFO("%s", method);
    Log::getInstance()->flush();

    // �����ִ�Сд�ıȽ��ַ����Ƿ���ȣ��ж����󷽷���GET����POST
    if (strcasecmp(method, "GET") == 0) {
        myMethod = GET;
    } else if (strcasecmp(method, "POST") == 0) {
        myMethod = POST;
        cgi = true; // POST������cgi��֤
    } else {
        return BAD_REQUEST;
    }
    // �ú���������Ŀո���Ʊ��ǰ׺�ĳ���
    str += strspn(str, " \t");
    myUrl = str; // ��¼URL
    str = strpbrk(str, " \t");
    if (!str) {
        return BAD_REQUEST;
    }
    *str++ = 0;
    str += strspn(str, " \t");
    myVersion = str; // ��¼HTTP�汾��

    LOG_INFO("%s", myVersion);
    Log::getInstance()->flush();

    // �жϰ汾��
    if (strcasecmp(myVersion, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }

    // �ж�URL�Ƿ����HTTPǰ׺
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
    // ֻ��һ��'/'��������Դҳ���׺
    if (strlen(myUrl) == 1) {
        strcat(myUrl, "judge.html");
    }

    LOG_INFO("%s", myUrl);
    Log::getInstance()->flush();

    myCheckState = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseHeader(char *text) {
    // �ж��ǿ��л�������ͷ
    if (text[0] == '\0') {
        // printf("blank line\n");                 // ����
        if (myContentLength != 0) {             // �ж��Ƿ���POST����
            myCheckState = CHECK_STATE_CONTENT; // POST��Ҫ����������
            return NO_REQUEST;
        }
        // GET����ֱ�ӷ��سɹ�
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) { // ��������ͷ�������ֶ�
        text += 11;
        text += strcspn(text, " \t"); // �����������еĿո���Ʊ��
        if (strcasecmp(text, "keep-alive") == 0) {
            myLinger = true; // ����keep-alive
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) { // ��������ͷ�����ݳ����ֶ�
        text += 15;
        text += strcspn(text, " \t");
        myContentLength = atoi(text);                // ��¼�����峤��
    } else if (strncasecmp(text, "Host:", 5) == 0) { // ��������ͷ��HOST�ֶ�
        text += 5;
        text += strcspn(text, " \t");
        myHost = text; // ��¼HOST
    } else {
        LOG_INFO("unknow header : %s.\n", text); // δ֪����ͷ
        Log::getInstance()->flush();
    }
    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseContent(char *text) {
    // �ж��������Ƿ�����
    if (myReadIndex >= myContentLength + myCheckedIndex) {
        text[myContentLength] = 0; // �������Ͻ�����ʶ��
        myString = text;           // ����POST�������ŵ����û���������
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

char *HttpConnection::getLine() {
    return myReadBuf + myStartLine; // ���ػ�δ������ַ���λ��ָ��
}

HttpConnection::HTTP_CODE HttpConnection::processRead() {
    // printf("processRead success\n");
    LINE_STATE lineState = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    // ���ôӻ���ȡ����һ�У��ж��Ƿ��ܽ����������н���
    while ((myCheckState == CHECK_STATE_CONTENT && lineState == LINE_OK) || (lineState = parseLine()) == LINE_OK) {
        // printf("success\n");
        text = getLine();             // �Ӷ���������ȡδ�����ַ���λ��
        myStartLine = myCheckedIndex; // �����Ѵ����ַ�λ��
        LOG_INFO("%s", text);         // �����־
        Log::getInstance()->flush();  // ˢ��
        switch (myCheckState) {       // �ж�����״̬
        case CHECK_STATE_REQUESTLINE: {
            ret = parseRequestLine(text); // ����������
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER: {
            ret = parseHeader(text); // ��������ͷ
            // printf("ret:%d\n", ret);
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            } else if (ret == GET_REQUEST) {
                return doRequest();
            }
            break;
        }
        case CHECK_STATE_CONTENT: {
            ret = parseContent(text); // ����������
            if (ret == GET_REQUEST) {
                return doRequest();
            }
            lineState = LINE_OPEN;
            break;
        }
        default:
            return INTERNAL_ERROR; // һ�㲻�ᴥ��
        }
    }
    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::doRequest() {
    strcpy(myFileName, docRoot); // ��ȡ��վ��Ŀ¼·��
    int len = strlen(docRoot);
    const char *p = strrchr(myUrl, '/'); // ��ȡ���һ��'/'λ��

    // ����cgi����¼��֤��ע����֤
    if (cgi == true && (*(p + 1) == '2' || *(p + 1) == '3')) {
        char flag = myUrl[1];
        char *myRealUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(myRealUrl, "/");
        strcat(myRealUrl, myUrl + 2);
        strncpy(myFileName + len, myRealUrl, fileNameLen - len - 1);
        free(myRealUrl);

        // ��ȡ�û���������
        string username, password;
        int i;
        for (i = 5; myString[i] != '&'; ++i) {
            username += myString[i];
        }
        int j = 0;
        for (i = i + 10; myString[i] != 0; ++i, ++j) {
            password += myString[i];
        }

        // ע��У��
        if (*(p + 1) == '3') {
            string sql = "insert into user(username,passwd) values('" + username + "','" + password + "')";
            // ����
            HttpConnection::mutex.lock();
            if (users.find(username) == users.end()) {
                int ret = mysql_query(connection, sql.c_str());         // �������ݿ�
                users.insert(pair<string, string>(username, password)); // �����û�����
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
            // ��¼У��
        } else if (*(p + 1) == '2') {
            if (users.find(username) != users.end() && users[username] == password) {
                strcpy(myUrl, "/welcome.html");
            } else {
                strcpy(myUrl, "/loginError.html");
            }
        }
    }

    // ��Ӷ�Ӧ����Դҳ��
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

    // û�ж�ȡȨ��
    if (!(myFileStat.st_mode & S_IROTH)) {
        return FORBIDDEN_REQUEST;
    }

    // ������ԴΪĿ¼
    if (S_ISDIR(myFileStat.st_mode)) {
        return BAD_REQUEST;
    }

    // ���ļ���ӳ�䵽�ڴ�
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
    if (myWriteIndex >= writeBufSize) { // д�����ݳ���д��������С
        return false;
    }
    // ����ɱ�����б�
    va_list valist;
    va_start(valist, format);
    // �����ݸ���format�ĸ�ʽд��д������
    int len = vsnprintf(myWriteBuf + myWriteIndex, writeBufSize - myWriteIndex - 1, format, valist);
    if (len >= (writeBufSize - myWriteIndex - 1)) {
        va_end(valist);
        return false;
    }
    // ����д������ָ��λ��
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
    // ��Ӧ����Ϊ��
    if (bytesToSend == 0) {
        modfd(epollfd, mySocketfd, EPOLLIN);
        init();
        return true;
    }
    while (1) {
        // ����Ӧ���ķ��ͳ�ȥ
        ret = writev(mySocketfd, myIovec, myIovecCount);

        // δ���ͳɹ�
        if (ret < 0) {
            // �жϻ������Ƿ�����
            if (errno == EAGAIN) {
                modfd(epollfd, mySocketfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        // �ֶ����´����͵�����λ��
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

        // ���ݷ������
        if (bytesToSend <= 0) {
            unmap();
            modfd(epollfd, mySocketfd, EPOLLIN);
            // �ж��Ƿ�Ϊkeep-alive
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