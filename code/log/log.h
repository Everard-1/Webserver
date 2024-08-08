#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

// 通用的日志记录宏，用于写入不同级别的日志信息
#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);
// 四个宏定义，主要用于不同类型的日志输出，也是外部使用日志的接口
// ...表示可变参数，__VA_ARGS__就是将...的值复制到这里
// 前面加上##的作用是：当可变参数的个数为0时，这里的##可以把把前面多余的","去掉,否则会编译出错。
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);    //调试级别的日志
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);     //信息级别的日志
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);     //警告级别的日志
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);    //错误级别的日志

using namespace std;

class Log {
public:
    // 初始化日志实例（日志级别、日志保存路径、日志文件后缀、阻塞队列最大容量）
    void init(int level, const char* path = "./log", 
                const char* suffix =".log",
                int maxQueueCapacity = 1024);

    static Log* Instance();
    static void FlushLogThread();    // 异步写日志公有方法，调用私有方法asyncWrite

    void write(int level, const char *format,...);  // 将输出内容按照标准格式整理
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }
    
private:
    Log();
    void AppendLogLevelTitle_(int level);
    virtual ~Log();
    void AsyncWrite_(); //异步写日志方法

private:
    static const int LOG_PATH_LEN = 256;    //日志文件最长文件名
    static const int LOG_NAME_LEN = 256;    //日志文件最长名字
    static const int MAX_LINES = 50000;     //日志文件最长日志条数

    const char* path_;      //文件路径
    const char* suffix_;    //文件后缀

    int MAX_LINES_;     // 最大日志行数

    int lineCount_;     //日志行数
    int toDay_;         //日志文件日期

    bool isOpen_;   
 
    Buffer buff_;       // 输出的内容，缓冲区
    int level_;         // 日志等级
    bool isAsync_;      // 是否开启异步日志

    FILE* fp_;                                //打开log的文件指针
    unique_ptr<BlockDeque<string>> deque_;    //阻塞队列
    unique_ptr<thread> writeThread_;          //写线程的指针
    mutex mtx_;                               //同步日志必需的互斥量
};

#endif //LOG_H