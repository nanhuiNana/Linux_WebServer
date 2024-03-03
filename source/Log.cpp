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
    // 懒汉模式，在调用时进行初始化，C++11后无需加锁，编译器会保证局部静态变量的线程安全
    static Log instance;
    return &instance;
}

bool Log::init(const char *fileName, int logBufSize, int logMaxLines, int queueMaxSize) {
    if (queueMaxSize >= 1) {
        // 如果日志队列最大长度大于0，开启异步模式
        myIsAsync = true;

        // 创建阻塞队列
        myLogQueue = new BlockQueue<string>(queueMaxSize);

        // 创建一个写线程，用于异步写入日志文件
        pthread_t pid;
        pthread_create(&pid, NULL, logWritePthreadCallback, NULL);
    }

    // 初始化各个成员
    myLogBufSize = logBufSize;
    myLogBuf = new char[myLogBufSize];
    memset(myLogBuf, 0, myLogBufSize);
    myLogMaxLines = logMaxLines;

    // 获取当前时间
    time_t t = time(NULL);
    struct tm *sysTm = localtime(&t);
    struct tm myTm = *sysTm;

    // 在fileName里查找最后一个'/'的位置
    const char *p = strrchr(fileName, '/');
    char logFileNameBuf[512] = {0};

    // 日志文件命名规则，年_月_日_文件名
    if (p == nullptr) {
        snprintf(logFileNameBuf, 511, "%d_%02d_%02d_%s", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, fileName);
    } else {
        // 拆分路径和文件名
        strcpy(myLogName, p + 1);
        strncpy(myDirName, fileName, p - fileName + 1);
        snprintf(logFileNameBuf, 511, "%s%d_%02d_%02d_%s", myDirName, myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, myLogName);
    }

    // 记录当前时间是哪一天
    myToday = myTm.tm_mday;

    // 以追加形式打开日志文件
    myLogFilePointer = fopen(logFileNameBuf, "a");
    if (myLogFilePointer == nullptr) {
        return false;
    }
    return true;
}

void Log::writeLog(int level, const char *format, ...) {
    // 获取当前时间
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sysTm = localtime(&t);
    struct tm myTm = *sysTm;

    // Log分级
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

    // 判断是否需要创建新的日志文件
    myMutex.lock();
    myLogCount++; // 更新日志行数
    // 判断是否是新的一天或者当前日志文件已经写满
    if (myToday != myTm.tm_mday || myLogCount % myLogMaxLines == 0) {
        char newLogFileNameBuf[512] = {0}; // 新日志文件名缓冲区
        fflush(myLogFilePointer);          // 刷新写入缓冲区
        fclose(myLogFilePointer);          // 关闭旧的日志文件
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

    // 可变参数定义初始化，在vsprintf时使用，作用：输入具体的日志内容
    va_list valist;
    va_start(valist, format);

    string logStr;
    // 将日志内容准备好
    myMutex.lock();
    // 每一行的开头格式
    int n = snprintf(myLogBuf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, myTm.tm_hour, myTm.tm_min, myTm.tm_sec, now.tv_usec, s);
    // 每一行的日志内容,format为写入日志的内容的格式，valist为需要写入的内容（可变参数）
    int m = vsnprintf(myLogBuf + n, myLogBufSize - n - 1, format, valist);
    // 加入换行和结束符
    myLogBuf[n + m] = '\n';
    myLogBuf[n + m + 1] = '\0';
    logStr = myLogBuf;
    myMutex.unlock();

    // 判断是否异步写入
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
    Log::getInstance()->asyncWriteLog(); // 调用异步写入函数
    return nullptr;
}

void Log::asyncWriteLog() {
    string logWords;
    // 从阻塞队列中取出一个日志，写入日志文件
    while (myLogQueue->pop(logWords)) {
        myMutex.lock();
        fputs(logWords.c_str(), myLogFilePointer);
        myMutex.unlock();
    }
}

void Log::flush() {
    myMutex.lock();
    // 强制刷新写入流缓冲区，将数据立即写入文件指针指向文件
    fflush(myLogFilePointer);
    myMutex.unlock();
}