<a name="wQb1U"></a>
# 封装Epoller搭建服务器
<a name="RwSt7"></a>
## Epoller
`Epoll`的API接口其实也就这么几个，我这里罗列一下：
```cpp
#include <sys/epoll.h>
// 创建一个新的epoll实例。在内核中创建了一个数据，这个数据中有两个比较重要的数据，一个是需要检测的文件描述符的信息（红黑树），还有一个是就绪列表，存放检测到数据发送改变的文件描述符信息（双向链表）。
int epoll_create(int size);
	- 参数：
		size : 目前没有意义了。随便写一个数，必须大于0
	- 返回值：
		-1 : 失败
		> 0 : 文件描述符，操作epoll实例的
            
typedef union epoll_data {
	void *ptr;
	int fd;
	uint32_t u32;
	uint64_t u64;
} epoll_data_t;

struct epoll_event {
	uint32_t events; /* Epoll events */
	epoll_data_t data; /* User data variable */
};
    
// 对epoll实例进行管理：添加文件描述符信息，删除信息，修改信息
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
	- 参数：
		- epfd : epoll实例对应的文件描述符
		- op : 要进行什么操作
			EPOLL_CTL_ADD: 添加
			EPOLL_CTL_MOD: 修改
			EPOLL_CTL_DEL: 删除
		- fd : 要检测的文件描述符
		- event : 检测文件描述符什么事情
            
// 检测函数
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
	- 参数：
		- epfd : epoll实例对应的文件描述符
		- events : 传出参数，保存了发生了变化的文件描述符的信息
		- maxevents : 第二个参数结构体数组的大小
		- timeout : 阻塞时间
			- 0 : 不阻塞
			- -1 : 阻塞，直到检测到fd数据发生变化，解除阻塞
			- > 0 : 阻塞的时长（毫秒）
	- 返回值：
		- 成功，返回发送变化的文件描述符的个数 > 0
		- 失败 -1
```
常见的Epoll检测事件：<br />EAGAIN: 我们时常会用一个while(true)死循环去接收缓冲区中客户端socket的连接，如果这个时候我们设置socket状态为非阻塞，那么accept如果在某个时间段没有接收到客户端的连接，因为是非阻塞的IO，accept函数会立即返回，并将errno设置为EAGAIN。关于EAGAIN可以看一下这篇博文：[【Linux Socket C++】为什么IO复用需要用到非阻塞IO？EAGAIN的简单介绍与应用_c++ eagain-CSDN博客](https://blog.csdn.net/qq_52572621/article/details/127792861)<br />下面是一些检测事件：
> POLLIN ：表示对应的文件描述符可以读（包括对端 SOCKET 正常关闭）；
> EPOLLOUT：表示对应的文件描述符可以写；
> EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
> EPOLLERR：表示对应的文件描述符发生错误；
> EPOLLHUP：表示对应的文件描述符被挂断；
> EPOLLET：将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
> EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里。
> 1. 客户端直接调用close，会触犯EPOLLRDHUP事件
> 2. 通过EPOLLRDHUP属性，来判断是否对端已经关闭，这样可以减少一次系统调用。


<a name="YvI82"></a>
## WebServer的设计
先来一张总体流程图：<br />![](https://cdn.nlark.com/yuque/0/2024/jpeg/27393008/1723365995897-6ed94bb0-dad0-46af-8987-190a33418127.jpeg#averageHue=%23c1b8ac&clientId=ue039ef59-d321-4&from=paste&id=ub1f5e616&originHeight=1706&originWidth=1280&originalType=url&ratio=1.375&rotation=0&showTitle=false&status=done&style=none&taskId=ucc5415ba-68dc-4fb1-bef5-46c1b8969ee&title=)
<a name="SfnQt"></a>
### 设计
按照软件分层设计的草图，WebServer设计目标为：

- 监听IO事件
   - 读事件
   - 写事件
- 处理超时连接 数据： int port_; 		//端口
```cpp
int timeoutMS_; 　　　　//毫秒MS,定时器的默认过期时间

bool isClose_; 　　　//服务启动标志

int listenFd_; 　　//监听文件描述符

bool openLinger_;　　//优雅关闭选项

char* srcDir_;　　　//需要获取的路径

uint32_t listenEvent_;　//初始监听描述符监听设置

uint32_t connectionEvent_;//初始连接描述符监听设置

std::unique_ptr timer_;　 		//定时器
std::unique_ptr threadpool_; 	//线程池
std::unique_ptr epoller_; 		//反应堆
std::unordered_map<int, HTTPconnection> users_;//连接队列
```
最下面几行参数就是我们实现的几大功能
<a name="cVopR"></a>
### WebServer类详解
<a name="ywCOk"></a>
#### 初始化
```cpp
threadpool_(new ThreadPool(threadNum))
InitSocket_();//初始化Socket连接
InitEventMode_(trigMode);//初始化事件模式
SqlConnPool::Instance()->Init();//初始化数据库连接池
Log::Instance()->init(logLevel, "./log", ".log", logQueSize);   
```
**创建线程池**：线程池的构造函数中会创建线程并且detach()<br />**初始化Socket的函数**`**InitSocket_();**`：C/S中，服务器套接字的初始化无非就是socket - bind - listen - accept - 发送接收数据这几个过程；函数执行到listen后，把前面得到的listenfd添加到epoller模型中，即把accept()和接收数据的操作交给epoller处理了。并且把该监听描述符设置为非阻塞。<br />**初始化事件模式函数**：InitEventMode_(trigMode);，将listenEvent_ 和 connEvent_都设置为EPOLLET模式。<br />**初始化数据库连接池**：SqlConnPool::Instance()->Init();创造单例连接池，执行初始化函数。<br />**初始化日志系统**：在初始化函数中，创建阻塞队列和写线程，并创建日志。
<a name="Wz6lb"></a>
#### 启动WebServer
接下来启动WebServer，首先需要设定`epoll_wait()`等待的时间，这里我们选择调用定时器的`GetNextTick()`函数，这个函数的作用是返回最小堆堆顶的连接设定的过期时间与现在时间的差值。这个时间的选择可以保证服务器等待事件的时间不至于太短也不至于太长。接着调用epoll_wait()函数，返回需要已经就绪事件的数目。这里的就绪事件分为两类：收到新的http请求和其他的读写事件。 这里设置两个变量fd和events分别用来存储就绪事件的文件描述符和事件类型。

1.收到新的HTTP请求的情况<br />在fd==listenFd_的时候，也就是收到新的HTTP请求的时候，调用函数`DealListen_();`处理监听，接受客户端连接；<br />2.已经建立连接的HTTP发来IO请求的情况<br />在events& EPOLLIN 或events & EPOLLOUT为真时，需要进行读写的处理。分别调用 `DealRead_(&users_[fd])`和`DealWrite_(&users_[fd])` 函数。
> 这里需要说明：`DealListen_()`函数并没有调用线程池中的线程，而`DealRead_(&users_[fd])`和`DealWrite_(&users_[fd])` 则都交由线程池中的线程进行处理了。

**这就是Reactor，读写事件交给了工作线程处理。**
<a name="hmoVZ"></a>
#### I/O处理的具体流程
`DealRead_(&users_[fd])`和`DealWrite_(&users_[fd]) `通过调用
```cpp
threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));     //读
threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));    //写
```
函数来取出线程池中的线程继续进行读写，而主进程这时可以继续监听新来的就绪事件了。
> 注意此处用`std::bind`将参数绑定，他可以将可调用对象将参数绑定为一个仿函数，绑定后的结果可以使用std::function保存，而且**bind绑定类成员函数时，第一个参数表示对象的成员函数的指针（所以上面的函数用的是**`&WebServer::OnRead_`），**第二个参数表示对象的地址**。

`OnRead_()`函数首先把数据从缓冲区中读出来(调用HttpConn的`read`,`read`调用`ReadFd`读取到读缓冲区BUFFER)，然后交由逻辑函数`OnProcess()`处理。这里多说一句，`process()`函数在解析请求报文后随即就生成了响应报文等待`OnWrite_()`函数发送，这一点我们前面谈到过的。<br />**这里必须说清楚OnRead_()和OnWrite_()函数进行读写的方法，那就是：分散读和集中写**
> 分散读（scatter read）和集中写（gatherwrite）具体来说是来自读操作的输入数据被分散到多个应用缓冲区中，而来自应用缓冲区的输出数据则被集中提供给单个写操作。 这样做的好处是：它们只需一次系统调用就可以实现在文件和进程的多个缓冲区之间传送数据，免除了多次系统调用或复制数据的开销。

OnWrite_()函数首先把之前根据请求报文生成的响应报文从缓冲区交给fd，传输完成后修改该fd的`events`.<br />OnProcess()就是进行业务逻辑处理（解析请求报文、生成响应报文）的函数了。<br />**一定要记住：“如果没有数据到来，epoll是不会被触发的”。当浏览器向服务器发出request的时候，epoll会接收到EPOLL_IN读事件，此时调用OnRead去解析，将fd(浏览器)的request内容放到读缓冲区，并且把响应报文写到写缓冲区，这个时候调用OnProcess()是为了把该事件变为EPOLL_OUT，让epoll下一次检测到写事件，把写缓冲区的内容写到fd。当EPOLL_OUT写完后，整个流程就结束了，此时需要再次把他置回原来的EPOLL_IN去检测新的读事件到来。**<br />以上这段话是整个WebServer的核心中的核心，一定要理解通透！！！
<a name="SLafW"></a>
## 代码
webserver.h<br />函数：

- 构造函数: 设置服务器参数　＋　初始化定时器／线程池／反应堆／连接队列
- 析构函数: 关闭listenFd_，　销毁　连接队列/定时器／线程池／反应堆
- 主函数start()
   - 创建端口，绑定端口，监听端口，　创建epoll反应堆，　将监听描述符加入反应堆
   - 等待事件就绪
      - 连接事件－－＞DealListen()
      - 写事件－－＞DealWrite()
      - 读事件－－＞DealRead()
   - 事件处理完毕，修改反应堆，再跳到２处循环执行
- DealListen: 新初始化一个ＨttpConnection对象
- DealWrite：　对应连接对象进行处理－－＞若处理成功，则监听事件转换成　读　事件
- DealRead：　 对应连接对象进行处理－－＞若处理成功，则监听事件转换成　写　事件