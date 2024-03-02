# 源文件目录
## Log.cpp
* 日志分级
  * Debug，调试代码时的输出，在系统实际运行时，一般不使用。
  * Warn，这种警告与调试时终端的warning类似，同样是调试代码时使用。
  * Info，报告系统当前的状态，当前执行的流程或接收的信息等。
  * Erro，输出系统的错误信息。
* va_list
  * 用于处理可变数量参数，va_list用于表示一个参数列表，这个参数列表包含了可变数量的参数，它使得函数能够接受不固定数量的参数，并对这些参数进行操作。
  * 使用va_start宏可以初始化va_list，将参数列表指向第一个可变参数。
  * 使用va_end宏可以结束对参数列表的操作。
  ~~~c
    va_list valist;
    va_start(valist, format);
    //format为写入日志的内容的格式，valist为需要写入的内容
    vsnprintf(myLogBuf + n, myLogBufSize - n - 1, format, valist);
    va_end(valist);
  ~~~
* 获取时间
  * time
  ~~~c
    time_t t = time(NULL);//获取当前时间的秒数，从1970.01.01至今的秒数
    struct tm* sysTm = localtime(&t);//转换为当地时间
    struct tm myTm = *sysTm;//解引用
  ~~~
  * gettimeofday
  ~~~c
    struct timeval now = {0, 0};//struct timeval是一个结构体，用来表示时间值，包括秒数和微秒数
    gettimeofday(&now, NULL);//获取当前时间
    time_t t = now.tv_sec;//获取当前时间的秒数
    struct tm* sysTm = localtime(&t);
    struct tm myTm = *sysTm;
  ~~~
* 异步写入
  * 创建一个写线程和一个阻塞队列，写入主函数通过判断异步开关来获取是否异步，如果是异步就往阻塞队列中添加待写入的日志，阻塞在队列上的写线程获取任务进行写入，典型的生产者消费者模型
