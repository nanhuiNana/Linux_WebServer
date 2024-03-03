#include "../inc/Log.h"

Log::Log() {
    myLogCount = 0;
    myIsAsync = false;
}

Log::~Log() {
    if (myLogFilePointer != nullptr) {
        fclose(myLogFilePointer);
    }
}

Log *Log::getInstance() {
    // ����ģʽ���ڵ���ʱ���г�ʼ����C++11������������������ᱣ֤�ֲ���̬�������̰߳�ȫ
    static Log instance;
    return &instance;
}

bool Log::init(const char *fileName, int logBufSize, int logMaxLines, int queueMaxSize) {
    if (queueMaxSize >= 1) {
        // �����־������󳤶ȴ���0�������첽ģʽ
        myIsAsync = true;

        // ������������
        myLogQueue = new BlockQueue<string>(queueMaxSize);

        // ����һ��д�̣߳������첽д����־�ļ�
        pthread_t pid;
        pthread_create(&pid, NULL, logWritePthreadCallback, NULL);
    }

    // ��ʼ��������Ա
    myLogBufSize = logBufSize;
    myLogBuf = new char[myLogBufSize];
    memset(myLogBuf, 0, myLogBufSize);
    myLogMaxLines = logMaxLines;

    // ��ȡ��ǰʱ��
    time_t t = time(NULL);
    struct tm *sysTm = localtime(&t);
    struct tm myTm = *sysTm;

    // ��fileName��������һ��'/'��λ��
    const char *p = strrchr(fileName, '/');
    char logFileNameBuf[512] = {0};

    // ��־�ļ�����������_��_��_�ļ���
    if (p == nullptr) {
        snprintf(logFileNameBuf, 511, "%d_%02d_%02d_%s", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, fileName);
    } else {
        // ���·�����ļ���
        strcpy(myLogName, p + 1);
        strncpy(myDirName, fileName, p - fileName + 1);
        snprintf(logFileNameBuf, 511, "%s%d_%02d_%02d_%s", myDirName, myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, myLogName);
    }

    // ��¼��ǰʱ������һ��
    myToday = myTm.tm_mday;

    // ��׷����ʽ����־�ļ�
    myLogFilePointer = fopen(logFileNameBuf, "a");
    if (myLogFilePointer == nullptr) {
        return false;
    }
    return true;
}

void Log::writeLog(int level, const char *format, ...) {
    // ��ȡ��ǰʱ��
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sysTm = localtime(&t);
    struct tm myTm = *sysTm;

    // Log�ּ�
    char s[16] = {0};
    switch (level) {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    // �ж��Ƿ���Ҫ�����µ���־�ļ�
    myMutex.lock();
    myLogCount++; // ������־����
    // �ж��Ƿ����µ�һ����ߵ�ǰ��־�ļ��Ѿ�д��
    if (myToday != myTm.tm_mday || myLogCount % myLogMaxLines == 0) {
        char newLogFileNameBuf[512] = {0}; // ����־�ļ���������
        fflush(myLogFilePointer);          // ˢ��д�뻺����
        fclose(myLogFilePointer);          // �رվɵ���־�ļ�
        if (myToday != myTm.tm_mday) {
            snprintf(newLogFileNameBuf, 511, "%s%d_%02d_%02d_%s", myDirName, myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, myLogName);
            myToday = myTm.tm_mday;
            myLogCount = 0;
        } else {
            snprintf(newLogFileNameBuf, 511, "%s%d_%02d_%02d_%s.%lld", myDirName, myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, myLogName, myLogCount / myLogMaxLines);
        }
        myLogFilePointer = fopen(newLogFileNameBuf, "a");
    }
    myMutex.unlock();

    // �ɱ���������ʼ������vsprintfʱʹ�ã����ã�����������־����
    va_list valist;
    va_start(valist, format);

    string logStr;
    // ����־����׼����
    myMutex.lock();
    // ÿһ�еĿ�ͷ��ʽ
    int n = snprintf(myLogBuf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, myTm.tm_hour, myTm.tm_min, myTm.tm_sec, now.tv_usec, s);
    // ÿһ�е���־����,formatΪд����־�����ݵĸ�ʽ��valistΪ��Ҫд������ݣ��ɱ������
    int m = vsnprintf(myLogBuf + n, myLogBufSize - n - 1, format, valist);
    // ���뻻�кͽ�����
    myLogBuf[n + m] = '\n';
    myLogBuf[n + m + 1] = '\0';
    logStr = myLogBuf;
    myMutex.unlock();

    // �ж��Ƿ��첽д��
    if (myIsAsync && !myLogQueue->full()) {
        myLogQueue->push(logStr);
    } else {
        myMutex.lock();
        fputs(logStr.c_str(), myLogFilePointer);
        myMutex.unlock();
    }

    va_end(valist);
}

void *Log::logWritePthreadCallback(void *args) {
    Log::getInstance()->asyncWriteLog(); // �����첽д�뺯��
    return nullptr;
}

void Log::asyncWriteLog() {
    string logWords;
    // ������������ȡ��һ����־��д����־�ļ�
    while (myLogQueue->pop(logWords)) {
        myMutex.lock();
        fputs(logWords.c_str(), myLogFilePointer);
        myMutex.unlock();
    }
}

void Log::flush() {
    myMutex.lock();
    // ǿ��ˢ��д����������������������д���ļ�ָ��ָ���ļ�
    fflush(myLogFilePointer);
    myMutex.unlock();
}