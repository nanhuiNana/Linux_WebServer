#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

class Log {
private:
    // 私有化构造函数，确保外界无法创建新实例
    Log();
    ~Log();

private:
    FILE* myLogFilePointer;  // 打开日志文件的指针
    long long myLogCount;    // 记录日志行数
    bool myIsAsync;          // 异步开关

public:
    // 创建一个公有静态方法获得实例，使用指针进行返回，避免不必要的拷贝构造
    static Log* get_instance() {
        // 懒汉模式，在调用时进行初始化，C++11后无需加锁，编译器会保证局部静态变量的线程安全
        static Log instance;
        return &instance;
    }
};

#endif