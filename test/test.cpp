#include "../code/log/log.h"
#include "../code/pool/threadpool.h"
#include <features.h>

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)    //定义gettid函数用于获取线程ID
#endif

void TestLog() {
    int cnt = 0, level = 0;
    Log::Instance()->init(level, "./testlog1", ".log", 0);  // 初始化日志系统，日志文件目录为 ./testlog1，文件后缀为 .log，最大文件大小为 0（无大小限制）
    for(level = 3; level >= 0; level--) {
        Log::Instance()->SetLevel(level);   //设置日志级别
        for(int j = 0; j < 10000; j++ ){
            for(int i = 0; i < 4; i++) {
                LOG_BASE(i,"%s 111111111 %d ============= ", "Test", cnt++);    //记录日志
            }
        }
    }
    cnt = 0;
    Log::Instance()->init(level, "./testlog2", ".log", 5000);   // 重新初始化日志系统，日志文件目录为 ./testlog2，文件后缀为 .log，最大文件大小为 5000 字节
    for(level = 0; level < 4; level++) {
    for(level = 0; level < 4; level++) {
        Log::Instance()->SetLevel(level);
        for(int j = 0; j < 10000; j++ ){
            for(int i = 0; i < 4; i++) {
                LOG_BASE(i,"%s 222222222 %d ============= ", "Test", cnt++);
            }
        }
    }
    }
}

void ThreadLogTask(int i, int cnt) {
    for(int j = 0; j < 10000; j++ ){
        LOG_BASE(i,"PID:[%04d]======= %05d ========= ", gettid(), cnt++);   //记录线程日志
    }
}

void TestThreadPool() {
    Log::Instance()->init(0, "./testThreadpool", ".log", 5000); // 初始化日志系统，日志文件目录为 ./testThreadpool，文件后缀为 .log，最大文件大小为 5000 字节
    ThreadPool threadpool(6);   //创建线程池，线程数为6
    for(int i = 0; i < 18; i++) {
        threadpool.AddTask(bind(ThreadLogTask, i % 4, i * 10000));  //添加任务到线程池
    }
    getchar();  // 等待用户输入，防止程序结束前线程池任务完成
}

int main() {
    TestLog();
    TestThreadPool();
}