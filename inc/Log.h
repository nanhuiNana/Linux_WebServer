#ifndef __LOG_H__
#define __LOG_H__

#include "BlockQueue.h"
#include "Locker.h"
#include "wrap.h"

// Log分级对外宏定义
#define LOG_DEBUG(format, ...) Log::getInstance()->writeLog(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::getInstance()->writeLog(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::getInstance()->writeLog(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::getInstance()->writeLog(3, format, ##__VA_ARGS__)

class Log {
private:
    // 私有化构造函数，确保外界无法创建新实例
    Log();

    ~Log();

    // 异步写入日志函数
    void asyncWriteLog();

private:
    char myDirName[128];            // 路径名
    char myLogName[128];            // 日志文件名
    FILE *myLogFilePointer;         // 打开日志文件的指针
    long long myLogCount;           // 记录日志行数
    int myLogMaxLines;              // 日志最大行数
    int myToday;                    // 记录当前时间是哪一天
    char *myLogBuf;                 // 日志缓冲区
    int myLogBufSize;               // 日志缓冲区大小
    bool myIsAsync;                 // 异步开关
    BlockQueue<string> *myLogQueue; // 日志队列
    Mutex myMutex;                  // 互斥锁

public:
    // 创建一个公有静态方法获得实例，使用指针进行返回，避免不必要的拷贝构造
    static Log *getInstance();

    // 初始化日志文件名、日志缓冲区大小、日志最大行数、日志队列最大长度
    bool init(const char *fileName, int logBufSize = 8192, int logMaxLines = 5000000, int queueMaxSize = 0);

    // 日志写入线程回调函数
    static void *logWritePthreadCallback(void *args);

    // 日志写入主函数
    void writeLog(int level, const char *format, ...);

    // 刷新缓冲区函数
    void flush();
};

#endif