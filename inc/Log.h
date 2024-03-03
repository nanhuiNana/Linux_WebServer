#ifndef __LOG_H__
#define __LOG_H__

#include "BlockQueue.h"
#include "Locker.h"
#include "wrap.h"

// Log�ּ�����궨��
#define LOG_DEBUG(format, ...) Log::getInstance()->writeLog(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::getInstance()->writeLog(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::getInstance()->writeLog(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::getInstance()->writeLog(3, format, ##__VA_ARGS__)

class Log {
private:
    // ˽�л����캯����ȷ������޷�������ʵ��
    Log();

    ~Log();

    // �첽д����־����
    void asyncWriteLog();

private:
    char myDirName[128];            // ·����
    char myLogName[128];            // ��־�ļ���
    FILE *myLogFilePointer;         // ����־�ļ���ָ��
    long long myLogCount;           // ��¼��־����
    int myLogMaxLines;              // ��־�������
    int myToday;                    // ��¼��ǰʱ������һ��
    char *myLogBuf;                 // ��־������
    int myLogBufSize;               // ��־��������С
    bool myIsAsync;                 // �첽����
    BlockQueue<string> *myLogQueue; // ��־����
    Mutex myMutex;                  // ������

public:
    // ����һ�����о�̬�������ʵ����ʹ��ָ����з��أ����ⲻ��Ҫ�Ŀ�������
    static Log *getInstance();

    // ��ʼ����־�ļ�������־��������С����־�����������־������󳤶�
    bool init(const char *fileName, int logBufSize = 8192, int logMaxLines = 5000000, int queueMaxSize = 0);

    // ��־д���̻߳ص�����
    static void *logWritePthreadCallback(void *args);

    // ��־д��������
    void writeLog(int level, const char *format, ...);

    // ˢ�»���������
    void flush();
};

#endif