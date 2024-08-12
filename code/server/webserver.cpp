#include "webserver.h"

using namespace std;

// 构造函数：初始化 WebServer 实例
WebServer::WebServer(
            int port, int trigMode, int timeoutMS,
            bool OptLinger, int sqlPort, const char* sqlUser, 
            const  char* sqlPwd, const char* dbName, int connPoolNum,
            int threadNum, bool openLog, int logLevel,
            int logQueSize):
            port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false),
            timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
    {
    //获取当前工作目录并设置资源目录
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);

    //初始化 HTTP 连接的静态成员
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;

    //初始化SQL连接池
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    //初始化事件模式和监听套接字
    InitEventMode_(trigMode);
    if(!InitSocket_()) { 
        isClose_ = true;
    }

    //如果启用日志，则初始化日志系统
    if(openLog) {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) { 
            LOG_ERROR("========== Server init error!=========="); 
        }
        else {  //日志：端口 延迟关闭；监听事件 连接事件；日志级别；资源目录；连接池 线程池大小
            LOG_INFO("===========    Server init!   ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

// 析构函数：释放资源
WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

// 根据触发模式初始化事件模式
void WebServer::InitEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;  //监听套接字关闭事件
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP; //EPOLLONESHOT：事件触发后只处理一次
    //初始化事件模式
    switch (trigMode)
    {
    case 0:
        break;                          //无特殊触发
    case 1:
        connEvent_ |= EPOLLET;          //连接事件——边沿触发
        break;
    case 2:
        listenEvent_ |= EPOLLET;        //监听事件——边沿触发
        break;
    case 3:
        listenEvent_ |= EPOLLET;        //监听、连接事件——边沿触发
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;        //默认：监听、连接事件——边沿触发
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);    // 设置 HTTP 连接的触发模式
}

// 启动 WebServer 实例
void WebServer::Start() {
    int timeMS = -1;  // epoll wait 超时时间设置为 -1，表示无限等待
    if(!isClose_) { 
        LOG_INFO("========== Server start =========="); 
    }
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();     // 获取下一次的超时等待事件
        }
        int eventCnt = epoller_->Wait(timeMS);  //已就绪事件数目
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i);   //获取事件描述符
            uint32_t events = epoller_->GetEvents(i);   //获取事件类型
            //如果事件描述符是监听套接字
            if(fd == listenFd_) {   
                DealListen_();  //处理监听套接字事件
            }
            //如果事件类型是连接关闭、挂起或错误
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {  
                assert(users_.count(fd) > 0);   //确保fd在用户连接中存在
                CloseConn_(&users_[fd]);    //关闭http连接
            }
            //如果事件类型是可读
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);     //处理读事件
            }
            //如果事件类型是可写
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);    //处理写事件
            } 
            else {  
                LOG_ERROR("Unexpected event");   // 如果事件类型不在预期之中，记录错误
            }
        }
    }
}

// 处理监听套接字的事件，接收新的连接并加入到epoller和定时器中
void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);     //接收新的连接请求
        if(fd <= 0) { 
            return;     //连接失败则返回
        }
        else if(HttpConn::userCount >= MAX_FD) {    //如果达到最大连接数
            SendError_(fd, "Server busy!");     //向客户端发送错误信息并关闭连接
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);   //连接未满，将新连接添加到客户端列表
    } while(listenEvent_ & EPOLLET);    //如果使用边沿触发模式，继续处理所有的连接请求
}

// 向客户端发送错误信息并关闭连接
void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);  //使用send函数向客户端发送错误信息
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);  //关闭套接字，释放资源
}

// 添加一个新客户端连接到服务器
void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);  //初始化客户端连接
    if(timeoutMS_ > 0) {     // 如果设置了超时时间，则将该连接添加到定时器中
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));  // 绑定一个超时回调函数，超时后调用 CloseConn_() 关闭连接
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);  // 将该文件描述符添加到epoll中，并设置监听事件为读事件和连接事件
    SetFdNonblock(fd);  // 设置文件描述符为非阻塞模式
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

// 关闭客户端连接并从 epoller 中删除相应的文件描述符
void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());   //从epoller中删除客户端的文件描述符
    client->Close();    //关闭客户端连接
}

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    int flags=fcntl(fd, F_GETFD, 0);     // 获取当前文件描述符的标志（flags）
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);  // 使用 fcntl 函数设置文件描述符的标志为非阻塞模式
}

// 处理读事件，主要逻辑是把 OnRead 加入线程池的任务队列中
void WebServer::DealRead_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);    //延长客户端连接的超时时间
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));     //把 OnRead 加入线程池的任务队列中
}

// 延长客户端连接的超时时间
void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) {    // 如果超时时间大于 0，调整定时器中的超时时间
        timer_->adjust(client->GetFd(), timeoutMS_); 
    }
}

// 处理客户端读事件
void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno); // 从客户端套接字读取数据
    if(ret <= 0 && readErrno != EAGAIN) {   // 处理读取错误或关闭连接
        CloseConn_(client);
        return;
    }
    OnProcess(client);   // 处理读取的数据（业务逻辑处理）
}

// 处理读（请求）数据的函数 
void WebServer::OnProcess(HttpConn* client) {
    // 首先调用process()进行逻辑处理
    if(client->process()) {     // 根据返回的信息重新将fd置为EPOLLOUT（写）或EPOLLIN（读）
        //读完事件就跟内核说可以写了
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);    // 响应成功，修改监听事件为写,等待OnWrite_()发送
    } 
    else {  //写完事件就跟内核说可以读了
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

// 处理写事件，主要逻辑是将 OnWrite 加入线程池的任务队列中
void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));    //将 OnWrite 加入线程池的任务队列中
}

// 处理客户端写事件
void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);   // 从客户端对象中写入数据
    if(client->ToWriteBytes() == 0) {   // 如果没有待写数据
        if(client->IsKeepAlive()) { // 如果连接保持活跃，则回到监听读事件
            OnProcess(client);  // 调用 OnProcess 处理后续操作
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {  // 如果缓冲区满了
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);    // 继续传输
            return;
        }
    }
    CloseConn_(client); // 处理其他情况或传输失败的情况，关闭连接
}

// 创建监听套接字并初始化相关设置
bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {     // 端口号应在 1024 到 65535 的范围内
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }

    // 配置地址结构体
    addr.sin_family = AF_INET;                // 地址族为 IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 绑定到所有网络接口
    addr.sin_port = htons(port_);            // 设置监听端口号

    struct linger optLinger = { 0 }; // 创建一个 linger 结构体对象并将其字段初始化为 0

    if(openLinger_) {
    /* 配置套接字在关闭时的行为，确保数据在关闭前被发送完毕或等待超时 */
    
    optLinger.l_onoff = 1;         // 启用延迟关闭选项 (SO_LINGER)，1 表示启用，0 表示禁用
    optLinger.l_linger = 1;        // 设置等待时间为 1 秒，指定在关闭套接字前等待数据发送完成的时间
    }


    // 创建监听套接字
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    // 设置延迟关闭选项
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);   // 关闭创建的套接字
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    // 允许端口在 TIME_WAIT 状态时被复用
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    // 绑定套接字到指定端口
    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    // 监听套接字，等待连接
    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    // 将监听套接字添加到 epoller 中，以便进行事件监控
    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }

    // 设置套接字为非阻塞模式
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}