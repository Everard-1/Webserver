#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>      // MySQL 数据库 API
#include <string>             // C++ 标准库字符串
#include <queue>              // C++ 标准库队列
#include <mutex>              // C++ 标准库互斥量
#include <semaphore.h>       // POSIX 信号量
#include <thread>             // C++ 标准库线程
#include "../log/log.h"      // 自定义日志类

using namespace std;

// SqlConnPool 类用于管理 SQL 连接池
class SqlConnPool {
public:
    // 获取 SqlConnPool 单例实例
    static SqlConnPool *Instance();
    
    // 从连接池获取一个连接
    MYSQL *GetConn();
    // 将连接释放回连接池
    void FreeConn(MYSQL * conn);
    // 获取当前空闲连接的数量
    int GetFreeConnCount();

    // 初始化连接池
    void Init(const char* host, int port,
              const char* user, const char* pwd, 
              const char* dbName, int connSize);
    // 关闭连接池并释放所有资源
    void ClosePool();

private:
    // 默认构造函数
    SqlConnPool();
    // 默认析构函数
    ~SqlConnPool();

    // 最大连接数
    int MAX_CONN_;
    // 当前正在使用的连接数
    int useCount_;
    // 当前空闲的连接数
    int freeCount_;
    
    // 存储 SQL 连接的队列
    queue<MYSQL *> connQue_;
    // 互斥量，用于保护 connQue_ 和其他共享数据的线程安全
    mutex mtx_;
    // 信号量，用于控制连接池的并发访问
    sem_t semId_;
};

#endif // SQLCONNPOOL_H
