#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"

/* SqlConnRAII 类用于管理 SQL 连接的生命周期
 * 通过 RAII 原则，确保连接在对象构造时获取，并在对象析构时释放
 */
class SqlConnRAII {
public:
    // 构造函数
    // 参数 sql: 指向 MYSQL 指针的指针，用于保存获取到的连接
    // 参数 connpool: 指向 SqlConnPool 对象的指针，用于从中获取和释放连接
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool) {
        assert(connpool);  // 确保传入的连接池指针有效
        *sql = connpool->GetConn();  // 从连接池中获取一个连接，并将其赋值给 sql
        sql_ = *sql;  // 保存连接的指针
        connpool_ = connpool;  // 保存连接池的指针
    }
    
    // 析构函数
    ~SqlConnRAII() {
        if (sql_) {  // 如果连接指针有效
            connpool_->FreeConn(sql_);  // 将连接归还给连接池
        }
    }
    
private:
    MYSQL *sql_;  // 保存的连接指针
    SqlConnPool* connpool_;  // 保存的连接池指针
};

#endif //SQLCONNRAII_H
