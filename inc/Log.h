#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

class Log {
private:
    // ˽�л����캯����ȷ������޷�������ʵ��
    Log();
    ~Log();

private:
    FILE* myLogFilePointer;  // ����־�ļ���ָ��
    long long myLogCount;    // ��¼��־����
    bool myIsAsync;          // �첽����

public:
    // ����һ�����о�̬�������ʵ����ʹ��ָ����з��أ����ⲻ��Ҫ�Ŀ�������
    static Log* get_instance() {
        // ����ģʽ���ڵ���ʱ���г�ʼ����C++11������������������ᱣ֤�ֲ���̬�������̰߳�ȫ
        static Log instance;
        return &instance;
    }
};

#endif