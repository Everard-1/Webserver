#include "sqlconnpool.h"
using namespace std;

SqlConnPool::SqlConnPool() {
    // 初始化连接数
    useCount_ = 0;
    freeCount_ = 0;
}

SqlConnPool::~SqlConnPool() {
    ClosePool();    //在对象销毁时关闭连接池
}

// SqlConnPool单例实例
SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;    //静态局部变量
    return &connPool;
}

// 初始化连接池
void SqlConnPool::Init(const char* host, int port,
            const char* user,const char* pwd, const char* dbName,
            int connSize = 10) {
    assert(connSize > 0);   //确保连接池有连接
    for (int i = 0; i < connSize; i++) {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);  //初始化MYSQL连接
        if (!conn) {
            LOG_ERROR("MySql init error!");     //记录初始化错误日志
            assert(conn);
        }
        conn = mysql_real_connect(conn, host,user, pwd,dbName, port, nullptr, 0); //连接数据库
        if (!conn) {
            LOG_ERROR("MySql Connect error!");  //记录连接错误日志
        }
        connQue_.push(conn);    //将连接加入队列
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);    //初始化信号量，允许connSize个连接
}

// 从连接池获取一个连接
MYSQL* SqlConnPool::GetConn() {
    MYSQL *conn = nullptr;
    if(connQue_.empty()){
        LOG_WARN("SqlConnPool busy!");  //记录警告日志
        return nullptr;
    }
    // 信号量-1，减少可用连接数
    sem_wait(&semId_);
    {
        lock_guard<mutex> locker(mtx_);
        conn = connQue_.front();    //获取队列第一个连接
        connQue_.pop();     //移除连接
    }
    return conn;
}

// 将连接释放回连接池，实际上没有关闭
void SqlConnPool::FreeConn(MYSQL* conn) {
    assert(conn);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(conn);    //连接插入队列尾部
    sem_post(&semId_);  //信号量+1，表示可用连接数增加
}

// 关闭连接池，释放资源
void SqlConnPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);
    //遍历队列，关闭所有连接
    while(!connQue_.empty()) {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    mysql_library_end();    //结束MYSQL库的使用
}

// 获取当前空闲连接数量
int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}
