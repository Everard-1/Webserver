#include "httpconn.h"
using namespace std;

const char* HttpConn::srcDir;
atomic<int> HttpConn::userCount;
bool HttpConn::isET;

//初始化httpConn对象
HttpConn::HttpConn() { 
    fd_ = -1;                      // 套接字文件描述符初始化为无效值
    addr_ = { 0 };                 // 客户端地址信息初始化为零
    isClose_ = true;               // 标记连接为关闭状态
};

// 关闭连接
HttpConn::~HttpConn() { 
    Close(); 
};

// 初始化连接，设置套接字文件描述符和客户端地址
void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);                // 确保文件描述符是有效的
    userCount++;                   // 增加活跃用户数
    addr_ = addr;                 // 设置客户端地址
    fd_ = fd;                     // 设置套接字文件描述符
    writeBuff_.RetrieveAll();     // 清空写缓冲区
    readBuff_.RetrieveAll();      // 清空读缓冲区
    isClose_ = false;             // 标记连接为开放状态
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

// 关闭http连接
void HttpConn::Close() {
    response_.UnmapFile();
    //关闭连接
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;    //减少活跃用户
        close(fd_);     //关闭套接字
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

// 获取套接字文件描述符
int HttpConn::GetFd() const {
    return fd_;
};

// 获取客户端地址
struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

// 获取客户端的Ip地址
const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);   // 讲IP地址转换为点分十进制字符串
}

// 获取客户端的端口号
int HttpConn::GetPort() const {
    return addr_.sin_port;
}

// 从套接字读取数据
ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);     //从套接字读取数据到缓冲区
        if (len <= 0) {
            break;  //直到没有数据或读取失败
        }
    } while (isET); //使用边沿触发模式，读取缓冲区到空
    return len;
}

// 向套接字写入数据，使用 writev 连续写函数
ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_); // 使用 writev 将数据从 iov 中写到套接字
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) {   //数据写完了
            break; 
        } 
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {   // 如果写入的字节数大于 iov_[0] 的长度
            //更新 iov_[1] 的偏移和长度
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();   //清空写缓冲区
                iov_[0].iov_len = 0;
            }
        }
        else {  // 写入的字节数小于等于iov[0]的长度
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);   //从写缓冲区中移除已写入的数据
        }
    } while(isET || ToWriteBytes() > 10240);     // 如果使用边沿触发模式或待写数据大于 10KB，继续写
    return len;
}

// 处理http请求并生成响应
bool HttpConn::process() {
    request_.Init();    // 初始化请求对象
    if(readBuff_.ReadableBytes() <= 0) {
        return false;   // 如果读取缓冲区没有数据，返回 false
    }
    else if(request_.parse(readBuff_)) {    // 解析请求数据
        LOG_DEBUG("%s", request_.path().c_str());  // 打印请求路径
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);  // 初始化响应对象
    } 
    else {
        response_.Init(srcDir, request_.path(), false, 400);  // 初始化错误响应对象
    }

    response_.MakeResponse(writeBuff_); // 生成响应报文放入写缓冲区
    // 设置 iov 结构体数组，准备写操作
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    // 如果响应中包含文件内容，设置第二个 iov 结构体
    if(response_.FileLen() > 0 && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}

