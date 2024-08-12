#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // 包含 fcntl() 函数的定义
#include <unistd.h>      // 包含 close() 函数的定义
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>     //包含 socket() 函数的定义
#include <netinet/in.h>     //包含 sockaddr_in 结构体的定义
#include <arpa/inet.h>      //包含 inet_pton() 函数的定义

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer {
public:
    // 构造函数：初始化WebServer实例
    /*参数说明：
    port: 服务器监听的端口号        trigMode: 触发模式（水平触发/边沿触发）       timeoutMS: 超时时间（单位：毫秒）
    OptLinger: 是否启用延迟关闭     sqlPort: SQL 数据库端口                     sqlUser: SQL 数据库用户
    sqlPwd: SQL 数据库密码          dbName: 数据库名称                          connPoolNum: 连接池大小
    threadNum: 线程池大小           openLog: 是否启用日志                       logLevel: 日志级别      
    logQueSize: 日志队列大小*/
    WebServer(
        int port, int trigMode, int timeoutMS, 
        bool OptLinger, int sqlPort, const char* sqlUser, 
        const  char* sqlPwd, const char* dbName, int connPoolNum, 
        int threadNum,bool openLog, int logLevel, 
        int logQueSize);

    // 析构函数：关闭WebServer实例
    ~WebServer();

    // 启动WebServer实例
    void Start();

private:
    bool InitSocket_();                             //初始化监听套接字
    void InitEventMode_(int trigMode);              //根据触发模式初始化事件模式
    void AddClient_(int fd, sockaddr_in addr);      //添加新的客户端连接
  
    void DealListen_();                             //处理监听套接字上的事件
    void DealWrite_(HttpConn* client);              //处理写操作
    void DealRead_(HttpConn* client);               //处理读操作

    void SendError_(int fd, const char*info);       //发送错误信息给客户端
    void ExtentTime_(HttpConn* client);             //延长客户连接的超时时间
    void CloseConn_(HttpConn* client);              //关闭客户端连接

    void OnRead_(HttpConn* client);                 //处理读事件
    void OnWrite_(HttpConn* client);                //处理写事件
    void OnProcess(HttpConn* client);               //处理http请求

    static const int MAX_FD = 65536;                //最大文件描述符数量

    static int SetFdNonblock(int fd);               //设置文件描述符为非阻塞模式

    int port_;                  // 服务器监听的端口号
    bool openLinger_;           // 是否启用延迟关闭
    int timeoutMS_;             // 超时时间（单位：毫秒）
    bool isClose_;              // 服务器是否关闭
    int listenFd_;              // 监听套接字文件描述符
    char* srcDir_;              // 资源目录
    
    uint32_t listenEvent_;      // 监听事件类型
    uint32_t connEvent_;        // 连接事件类型
   
    std::unique_ptr<HeapTimer> timer_;       // 定时器对象
    std::unique_ptr<ThreadPool> threadpool_; // 线程池对象
    std::unique_ptr<Epoller> epoller_;       // epoll 对象
    std::unordered_map<int, HttpConn> users_; // 存储客户端连接的映射
};

#endif //WEBSERVER_H