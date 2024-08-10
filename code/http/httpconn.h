#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev，提供高效的读写接口
#include <arpa/inet.h>   // sockaddr_in，定义网络地址结构体
#include <stdlib.h>      // atoi()，转换字符串到整数
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

/*
HttpConn 类负责管理和处理与客户端的 HTTP 连接。
它包括读取数据、解析请求、生成响应，并将响应写回客户端。
*/
class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    //初始化连接，设置套接字文件描述符和客户端地址
    void init(int sockFd, const sockaddr_in& addr);

    // 从套接字中读数据
    ssize_t read(int* saveErrno);
    // 向套接字中写数据
    ssize_t write(int* saveErrno);

    // 关闭HTTP连接
    void Close();

    // 获取套接字文件描述符
    int GetFd() const;

    // 获取客户端端口号
    int GetPort() const;
    // 获取客户端IP地址
    const char* GetIP() const;
    // 获取客户端地址信息
    sockaddr_in GetAddr() const;
    
    // 处理http请求并生成响应
    bool process();

    // 计算待写入的字节数
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    // 检查是否需要保持活动状态
    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    // 是否使用边缘触发（Edge Triggered）模式
    static bool isET;
    // 静态文件根目录
    static const char* srcDir;
    // 当前活跃的用户数
    static std::atomic<int> userCount;  // 支持多线程环境下的原子操作
    
private:
    int fd_;    
    struct  sockaddr_in addr_;

    bool isClose_;
    
    int iovCnt_;    // iovec 结构体数组的元素数量，用于分散读写操作
    struct iovec iov_[2];   // iovec 结构体数组，描述读写操作的缓冲区
    
    Buffer readBuff_; // 读缓冲区
    Buffer writeBuff_; // 写缓冲区

    HttpRequest request_;    // HTTP 请求对象，负责解析和存储客户端的请求信息
    HttpResponse response_;       // HTTP 响应对象，负责生成和处理 HTTP 响应
};


#endif //HTTP_CONN_H