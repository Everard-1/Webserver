项目功能

- 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
- 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
- 利用标准库容器封装char，实现自动增长的缓冲区；
- 基于小根堆实现的定时器，关闭超时的非活动连接；
- 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；
- 利用RAII机制实现了数据库连接池，减少数据库连接建立与关闭的开销，同时实现了用户注册登录功能。
- 增加logsys,threadpool测试单元(todo: timer, sqlconnpool, httprequest, httpresponse)

项目架构：<br />![](https://cdn.nlark.com/yuque/0/2024/jpeg/27393008/1723444573730-3c04e7d0-4978-4813-877b-cb36f8a12b7d.jpeg)
<a name="AS0L5"></a>
# 项目总述
![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723444630277-db317875-5d35-48f0-9799-e16ff1898a78.png#averageHue=%23c0bab1&clientId=ub4d37c3c-d40f-4&from=paste&id=ue74b00bc&originHeight=448&originWidth=730&originalType=url&ratio=1.6500000953674316&rotation=0&showTitle=false&status=done&style=none&taskId=u94063435-7529-4e90-918c-ba5108b7517&title=)
<a name="y16Vg"></a>
# Buffer类
Buffer类的设计源自陈硕大佬的muduo网络库。由于muduo库使用的是非阻塞I/O模型，即每次send()不一定会发送完，没发完的数据要用一个容器进行接收，所以必须要实现应用层缓冲区。<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723041113538-9e72480b-3076-47e0-8a24-feb9c2da3d40.png#averageHue=%23b2aba1&clientId=ue6a1ffd3-6d1b-4&from=paste&id=pIBA2&originHeight=372&originWidth=1053&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=udec9665e-4416-4bb8-bb5e-175c64ade7e&title=)<br />这里贴一张我对这个类的总结，其中`ReadFd`和`WriteFd`应该是重点，外部调用接口也是会调用这两个。<br />另外就是几个下标之间的关系，根据这个图来好好看一下，一定不要弄混了，写是往writeIndex的指针写，读取则是从readIndex的指针读。当写入fd的时候，是将buffer中的readable写入fd；当需要读取fd的内容时，则是读到writable的位置，注意区分写和读。
<a name="ANPSZ"></a>
## 主要实现方法
在WebServer中，客户端连接发来的HTTP请求（放到conn的读缓冲区）以及回复给客户端所请求的响应报文（放到conn的写缓冲区），都需要通过缓冲区来进行。我们以vector容器作为底层实体，在它的上面封装自己所需要的方法来实现一个自己的buffer缓冲区，满足读写的需要。<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723040869472-ab7b4279-a0bf-43a1-8e9b-3e9a74383db8.png#averageHue=%23f7f7f7&clientId=ue6a1ffd3-6d1b-4&from=paste&id=uc3917d64&originHeight=161&originWidth=625&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u62da8ed8-f3a1-44c5-bd59-c395cdd53ae&title=)

- **buffer的存储实体**

缓冲区的最主要需要是读写数据的存储，也就是需要一个存储的实体。自己去写太繁琐了，直接用vector来完成。也就是buffer缓冲区里面需要一个：
```cpp
std::vector<char>buffer_;
```

- **buffer所需要的变量**

由于buffer缓冲区既要作为读缓冲区，也要作为写缓冲区，所以我们既需要指示当前读到哪里了，也需要指示当前写到哪里了。所以在buffer缓冲区里面设置变量：
```cpp
std::atomic<std::size_t>readPos_;
std::atomic<std::size_t>writePos_;
```
分别指示当前读写位置的下标。其中atomic是一种原子类型，可以保证在多线的情况下，安全高性能得执行程序，更新变量。

- **buffer所需要的方法**

读写接口<br />缓冲区最重要的就是读写接口，主要可以分为与客户端直接IO交互所需要的读写接口，以及收到客户端HTTP请求后，我们在处理过程中需要对缓冲区的读写接口。<br />与客户端直接I/O得读写接口（httpconn中就是调用的该接口。）：
```cpp
ssize_t ReadFd();
ssize_t WriteFd();
```
这个功能直接用read()/write()、readv()/writev()函数来实现。从某个连接接受数据的时候，有可能会超过vector的容量，所以我们用readv()来分散接受来的数据。
<a name="WU6Rq"></a>
## 创新点
问题的提出：在非阻塞网络编程中，如何设计并使用缓冲区？一方面我们希望**减少系统调用**，一次读的数据越多越划算，那么似乎应该准备一个大的缓冲区。另一方面，我们系统**减少内存占用**。如果有 10k 个连接，每个连接一建立就分配 64k 的读缓冲的话，将占用 640M 内存，而大多数时候这些缓冲区的使用率很低。muduo 用 readv 结合栈上空间巧妙地解决了这个问题。<br />在栈上准备一个 65536 字节的 stackbuf，然后利用 readv() 来读取数据，iovec 有两块，第一块指向 muduo Buffer 中的 writable 字节，另一块指向栈上的 stackbuf。这样如果读入的数据不多，那么全部都读到 Buffer 中去了；如果长度超过 Buffer 的 writable 字节数，就会读到栈上的 stackbuf 里，然后程序再把 stackbuf 里的数据 append 到 Buffer 中。<br />这么做利用了临时栈上空间，避免开巨大 Buffer 造成的内存浪费，也避免反复调用 read() 的系统开销（通常一次 readv() 系统调用就能读完全部数据）。
> 总结：
> 代码中主要实现的就是写入fd和读取fd时，buffer缓冲区的可读可写指针位置的变化、可读可写区域大小的变化，以及我们在数据读取不完的情况下将剩余数据加入到我们的栈区。

<a name="WtW7Z"></a>
# 实现日志系统
<a name="besOt"></a>
## 预备知识
<a name="AATWP"></a>
### 单例模式
单例模式是最常用的设计模式之一，目的是保证一个类只有一个实例，并提供一个他的全局访问点，该实例被所有程序模块共享。此时需要把该类的构造和析构函数放入private中。<br />单例模式有两种实现方法，一种是懒汉模式，另一种是饿汉模式。

- **懒汉模式**： 顾名思义，非常的懒，只有当调用getInstance的时候，才会去初始化这个单例。其中在C++11后，不需要加锁，直接使用函数内局部静态对象即可。
- **饿汉模式**： 即迫不及待，在程序运行时立即初始化。饿汉模式不需要加锁，就可以实现线程安全，原因在于，在程序运行时就定义了对象，并对其初始化。之后，不管哪个线程调用成员函数getinstance()，都只不过是返回一个对象的指针而已。所以是线程安全的，不需要在获取实例的成员函数中加锁。
<a name="Ga8Vu"></a>
### 同步/异步日志

- **同步**日志：日志写入函数与工作线程串行执行，由于涉及到I/O操作，当单条日志比较大的时候，同步模式会阻塞整个处理流程，服务器所能处理的并发能力将有所下降，尤其是在峰值的时候，写日志可能成为系统的瓶颈。
- **异步**日志：将所写的日志内容先存入阻塞队列中，写线程从阻塞队列中取出内容，写入日志。

考虑到文件IO操作是非常慢的，所以采用异步日志就是先将内容存放在内存里，然后日志线程有空的时候再写到文件里。<br />日志队列是什么呢？他的需求就是时不时会有一段日志放到队列里面，时不时又会被取出来一段，这不就是经典的生产者消费者模型吗？所以还需要加锁、条件变量等来帮助这个队列实现。
<a name="Rrr5G"></a>
## 日志系统的实现
实现流程：<br />![](https://cdn.nlark.com/yuque/0/2024/jpeg/27393008/1723126212855-49c5b3a9-085c-4181-a26f-1096fffd42b5.jpeg#averageHue=%23f8f8f8&clientId=u92387165-256f-4&from=paste&id=u3e5ecfa6&originHeight=339&originWidth=1212&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u4ef5fb80-c5cc-4f42-927a-bd78753d22b&title=)
<a name="NF5Sl"></a>
### 日志系统实现流程
1、使用单例模式（局部静态变量方法）获取实例Log::getInstance()。<br />2、通过实例调用Log::getInstance()->init()函数完成初始化，若设置阻塞队列大小大于0则选择异步日志，等于0则选择同步日志，更新isAysnc变量。<br />3、通过实例调用write_log()函数写日志，首先根据当前时刻创建日志（前缀为时间，后缀为".log"，并更新日期today和当前行数lineCount。<br />4、在write_log()函数内部，通过isAsync变量判断写日志的方法：如果是异步，工作线程将要写的内容放进阻塞队列中，由写线程在阻塞队列中取出数据，然后写入日志；如果是同步，直接写入日志文件中。<br />该函数采用不定参数的形式：
```cpp
va_list vaList;
va_start(vaList, format);
int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
va_end(vaList);
```
<a name="lVT0D"></a>
### 阻塞队列BlockDeque
阻塞队列采用deque实现。<br />若`MaxCapacity` 阻塞队列大小为0，则为同步日志，不需要阻塞队列。<br />内部有生产者消费者模型，搭配锁、条件变量使用。<br />其中，消费者防止任务队列为空，生产者防止任务队列满。
<a name="xYRe5"></a>
### 日志的分级和分文件
**分级情况**：

- Debug，调试代码时的输出，在系统实际运行时，一般不使用。
- Warn，这种警告与调试时终端的warning类似，同样是调试代码时使用。
- Info，报告系统当前的状态，当前执行的流程或接收的信息等。
- Erro，输出系统的错误信息

**分文件情况**：

1. 按天分，日志写入前会判断当前today是否为创建日志的时间，若为创建日志时间，则写入日志，否则按当前时间创建新的log文件，更新创建时间和行数。
2. 按行分，日志写入前会判断行数是否超过最大行限制，若超过，则在当前日志的末尾加lineCount / MAX_LOG_LINES为后缀创建新的log文件。

<a name="L3C2U"></a>
# 线程池和连接池
流程图：<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723185420019-83f1951c-3413-4039-b809-b033fa8ffc8d.png#averageHue=%23b0ada6&clientId=uecc1cac5-0408-4&from=paste&id=uf1563c1d&originHeight=706&originWidth=822&originalType=url&ratio=1.25&rotation=0&showTitle=false&status=done&style=none&taskId=u6a4394f0-d9f5-4997-8442-93516d17142&title=)
<a name="pV2NQ"></a>
## 线程池
使用线程池可以减少线程的销毁，而且如果不使用线程池的话，来一个客户端就创建一个线程。比如有1000，这样线程的创建、线程之间的调度也会耗费很多的系统资源，所以采用线程池使程序的效率更高。 线程池就是项目启动的时候，就先把线程池准备好。

一般线程池的实现是通过生产者消费者模型来的：<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723185059964-ae3be438-a58f-4f37-8ec8-bbcab18a5e13.png#averageHue=%23f2f2f2&clientId=uecc1cac5-0408-4&from=paste&height=292&id=u1b7c12be&originHeight=438&originWidth=1159&originalType=binary&ratio=1.25&rotation=0&showTitle=false&size=45451&status=done&style=none&taskId=u76888254-0220-4745-b756-535ff6f9c03&title=&width=772.6666666666666)
> 线程同步问题涉及到了互斥量、条件变量。 在代码中，将互斥锁、条件变量、关闭状态、工作队列封装到了一起，通过一个共享智能指针来管理这些条件。
> 鉴于本人对匿名函数不是很熟悉，所以在写的时候又去学习了一遍匿名函数。关于匿名函数的使用，参考这篇博客：实例讲解[C++中lambda表达式](https://blog.csdn.net/huangshanchun/article/details/47155859)
> 在匿名函数中还用到了move，也是C++11的一个新特性，该函数的作用是将左值转换为右值，资产转移，该函数是为性能而生的。

<a name="g6cjV"></a>
## 连接池
<a name="ZXbJa"></a>
### RAII
什么是RAII？<br />RAII是C++语言的一种管理资源、避免泄漏的惯用法。利用的就是C++构造的对象最终会被销毁的原则。RAII的做法是使用一个对象，在其构造时获取对应的资源，在对象生命期内控制对资源的访问，使之始终保持有效，最后在对象析构的时候，释放构造时获取的资源。

为什么要使用RAII？<br />在计算机系统中，资源是数量有限且对系统正常运行具有一定作用的元素。比如：网络套接字、互斥锁、文件句柄和内存等等，它们属于系统资源。由于系统的资源是有限的，就好比自然界的石油，铁矿一样，不是取之不尽，用之不竭的，所以，我们在编程使用系统资源时，都必须遵循一个步骤：<br />1 申请资源；<br />2 使用资源；<br />3 释放资源。<br />第一步和第三步缺一不可，因为资源必须要申请才能使用的，使用完成以后，必须要释放，如果不释放的话，就会造成资源泄漏。

我们常用的智能指针如`unique_ptr`、锁`lock_guard`就是采用了RAII的机制。<br />在使用多线程时，经常会涉及到共享数据的问题，C++中通过实例化std::mutex创建互斥量，通过调用成员函数lock()进行上锁，unlock()进行解锁。不过这意味着必须记住在每个函数出口都要去调用unlock()，也包括异常的情况，这非常麻烦，而且不易管理。C++标准库为互斥量提供了一个RAII语法的模板类std::lock_guard，其会在构造函数的时候提供已锁的互斥量，并在析构的时候进行解锁，从而保证了一个已锁的互斥量总是会被正确的解锁。
> C++11新特性：
> 尽量用make_shared代替new去配合shared_ptr使用，因为如果通过new再传递给shared_ptr，内存是不连续的，会造成内存碎片化
> function<void()>是一个可调用对象。他可以取代函数指针的作用，因为它可以延迟函数的执行，特别适合作为回调函数使用。它比普通函数指针更加的灵活和便利。

<a name="KykWE"></a>
### 连接池底层实现
连接池有用std::list写的，也有用std::queue写的，我用queue写。<br />为什么要使用连接池？

- 由于服务器需要频繁地访问数据库，即需要频繁创建和断开数据库连接，该过程是一个很耗时的操作，也会对数据库造成安全隐患。
- 在程序初始化的时候，集中创建并管理多个数据库连接，可以保证较快的数据库读写速度，更加安全可靠。

在连接池的实现中，使用到了信号量来管理资源的数量；而锁的使用则是为了在访问公共资源的时候使用。所以说，无论是条件变量还是信号量，都需要锁。<br />不同的是，信号量的使用要先使用信号量sem_wait再上锁，而条件变量的使用要先上锁再使用条件变量wait。<br />具体可参考：[Linux条件变量pthread_condition细节（为何先加锁，pthread_cond_wait为何先解锁，返回时又加锁）_条件变量为什么要阻塞的时候先解锁-CSDN博客](https://blog.csdn.net/modi000/article/details/104779937)
<a name="bgoe5"></a>
## 单元测试
在代码没问题的情况下进行单元测试
```bash
cd test
make
./test
```
![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723190761136-d8af78a0-5aa4-487e-93be-90450b4c31f6.png#averageHue=%23817449&clientId=uecc1cac5-0408-4&from=paste&height=920&id=u3b70f299&originHeight=1380&originWidth=2560&originalType=binary&ratio=1.25&rotation=0&showTitle=false&size=643684&status=done&style=none&taskId=u8aeae6af-6951-41c2-b304-a23c03511f2&title=&width=1706.6666666666667)
<a name="vbkxN"></a>
# 有限状态机
http报文的解析主要是使用**有限状态机**来实现，解析过程中需要使用到**正则表达式**来得到各个部分的数据。下面是大致的流程图：<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723270350963-8ac076bc-09a6-4a3b-aad5-94c0f0b9119f.png#averageHue=%23a18f7f&clientId=uededeb4d-7da6-4&from=paste&id=u358c484d&originHeight=739&originWidth=861&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u7bffb670-35d1-400d-958e-0f1417f0712&title=)
<a name="kL6cW"></a>
## 简单介绍正则表达式

- `used?`

?代表前面的一个字符可以出现0次或1次，说简单点就是d可有可无。

- `ab*c`

*代表前面的一个字符可以出现0次或多次

- `ab+c`

+会匹配出现1次以上的字符

- `ab{6}c`

指定出现的次数，比如这里就是abbbbbbc，同理还有ab{2,6}c：限定出现次数为2~6次；ab{2,}c为出现2次		以上

- `(ab)+`

这里的括号会将ab一起括起来当成整体来匹配

- `a (cat|dog)`

或运算：括号必不可少，可以匹配a cat和a dog

- `[abc]+`

或运算：要求匹配的字符只能是[]里面的内容；[a-z]匹配小写字符；[a-zA-Z]：匹配所有字母；a-zA-Z0-9：匹配所有字母和数字

- `.*`

. 表示 匹配除换行符 \n 之外的任何单字符，*表示零次或多次。所以.*在一起就表示任意字符出现零次或多次。

- `^`限定开头；取反：[^a]表示“匹配除了a的任意字符”，只要不是在[]里面都是限定开头
- `$`匹配行尾。如^a只会匹配行首的a，a$只会匹配行尾的a
- `?`

表示将贪婪匹配->懒惰匹配。就是匹配尽可能少的字符。就意味着匹配任意数量的重复，但是在能使整个匹配成功的前提下使用最少的重复。
<a name="EeYVj"></a>
## http请求报文
**HTTP请求报文包括请求行、请求头部、空行和请求数据四个部分。**

 ![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723269925383-ded73b43-3d3f-46dc-8c5e-aa8773f6df4b.png#averageHue=%23dadada&clientId=uededeb4d-7da6-4&from=paste&id=uc03ea233&originHeight=165&originWidth=466&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u9c747a08-4526-4f91-b9ba-b7c2782861c&title=)<br />以下是百度的请求包：
> GET / HTTP/1.1
> Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,/;q=0.8,application/signed-exchange;v=b3;q=0.9
> Accept-Encoding: gzip, deflate, br
> Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
> Connection: keep-alive
> Host: www.baidu.com
> Sec-Fetch-Dest: document
> Sec-Fetch-Mode: navigate
> Sec-Fetch-Site: none
> Sec-Fetch-User: ?1
> Upgrade-Insecure-Requests: 1
> User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.4951.41 Safari/537.36 Edg/101.0.1210.32
> sec-ch-ua: " Not A;Brand";v=“99”, “Chromium”;v=“101”, “Microsoft Edge”;v=“101”
> sec-ch-ua-mobile: ?0
> sec-ch-ua-platform: “Windows”

> **上面只包括请求行、请求头和空行，请求数据为空。请求方法是GET，协议版本是HTTP/1.1；请求头是键值对的形式。**

<a name="ghKIl"></a>
## 有限状态机
整个解析过程由`parse()`函数完成；函数根据http请求报文分别调用三个不同函数
```cpp
ParseRequestLine_();//解析请求行
ParseHeader_();//解析请求头
ParseBody_();//解析请求体
```
![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723270126376-8322112a-f247-4a85-b5be-7921848a28dd.png#averageHue=%231e1e1e&clientId=uededeb4d-7da6-4&from=paste&height=431&id=uf809f42b&originHeight=646&originWidth=744&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=36884&status=done&style=none&taskId=u29bcbd1b-5f74-4261-9495-12637a98ea7&title=&width=496)<br />三个函数对请求行、请求头和数据体进行解析。当然解析请求体的函数还会调用`ParsePost_()`，因为Post请求会携带请求体。<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723270208594-135b739c-a239-4434-a445-f2cdc477b603.png#averageHue=%23f9f8f8&clientId=uededeb4d-7da6-4&from=paste&id=u8d3e5e75&originHeight=354&originWidth=841&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u08dc4d82-3ec1-42d2-8fc4-15059dc9773&title=)
<a name="u7hK2"></a>
# HTTP连接
<a name="qAlC7"></a>
## http响应报文
响应报文组成：
```cpp
HTTP/1.1 200 OK
Date: Fri, 22 May 2009 06:07:21 GMT
Content-Type: text/html; charset=UTF-8
\r\n
<html>
      <head></head>
      <body>
            <!--body goes here-->
      </body>
</html>
```

- 状态行，由HTTP协议版本号， 状态码， 状态消息 三部分组成。 第一行为状态行，（HTTP/1.1）表明HTTP版本为1.1版本，状态码为200，状态消息为OK。
- 消息报头，用来说明客户端要使用的一些附加信息。 第二行和第三行为消息报头，Date:生成响应的日期和时间；Content-Type:指定了MIME类型的HTML(text/html),编码类型是UTF-8。
- 空行，消息报头后面的空行是必须的。**它的作用是通过一个空行，告诉服务器头部到此为止**。
- 响应正文，服务器返回给客户端的文本信息。空行后面的html部分为响应正文。
<a name="xQvTP"></a>
## 实现响应报文步骤
在`HttpResponse.cpp`中，主要是靠`MakeResponse(Buffer& buff)`这个函数实现的，分为三个步骤：1. 添加状态行；2. 添加头部；3. **添加响应报文**，其中，第三步涉及到内存映射。<br />我们在`HttpRequest.cpp`中生成了请求报文，请求报文里面包括了我们要请求的页面（path），响应报文的过程就是根据请求报文写出响应头，将path上的文件放在响应体，然后发送给客户端（浏览器）。
<a name="Dr0Aw"></a>
### 生成状态码
```cpp
if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }
```
关于stat的使用，参考这篇博文：[stat函数-CSDN博客](https://blog.csdn.net/Dustinthewine/article/details/126673326)
<a name="gYmXP"></a>
### 添加状态行和头部
这里主要就是一些响应报文基本的内容，用`buff.Append()`添加进去即可。<br />状态行只需要添加文件对应状态的状态码即可<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723294453394-56f46875-7fbe-4470-bb5f-a8db774c530e.png#averageHue=%231f1e1e&clientId=u38251e1b-1caf-4&from=paste&height=296&id=u014f10a5&originHeight=489&originWidth=1153&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=73945&status=done&style=none&taskId=u4dbdfd2f-ad28-4aa0-9074-433d9537a9a&title=&width=698.7878383990294)<br />http头部也只需添加对应的文件类型和长度等信息<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723294619886-be8cfde1-3919-405c-b289-87123a74f73c.png#averageHue=%2321201f&clientId=u38251e1b-1caf-4&from=paste&height=304&id=u08f9457c&originHeight=502&originWidth=1027&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=62473&status=done&style=none&taskId=udebfd815-fed6-425e-b7dd-a6eba590cd7&title=&width=622.4242064490921)
<a name="qsY9s"></a>
### 添加响应正文
```cpp
// 添加响应内容（文件或错误页面）
void HttpResponse::AddContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);   //打开文件
    if(srcFd < 0) { 
        ErrorContent(buff, "File NotFound!");   //文件打开失败
        return; 
    }

    /* 将文件映射到内存，提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");   //映射失败，返回错误内容
        return; 
    }
    mmFile_ = (char*)mmRet; // 设置内存映射文件指针
    close(srcFd);   //关闭文件描述符
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}
```

1. 首先需要打开这个文件
2. 用`mmap`这个函数映射到内存，可以提高文件的访问速度
3. 映射过后，就可以将该文件描述符关闭了
4. 将该内存映射的指针加入`buff`，至此，报文的所有内容就都在buff中了，后续读取到浏览器上，浏览器解析渲染就得到我们所看到的内容了。
<a name="EQUaO"></a>
## http连接
<a name="Ihdci"></a>
### 整体分析
一个HTTP连接要实现的功能就是：

1. 读取请求
2. 解析请求
3. 生成响应
4. 发送响应

然后浏览器就可以进行解析渲染了。<br />先看一下大致的流程图：<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723294809290-53e71fa4-7917-4631-af9a-d7deed485b59.png#averageHue=%23bbb1a7&clientId=u38251e1b-1caf-4&from=paste&id=u92e83b31&originHeight=725&originWidth=708&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u200d9d6d-08a4-4551-b6ad-dc5067bdb0d&title=)<br />在一个HttpConn中，有两个缓冲区，一个用来读，一个用来写，还分别有个**request_** （将请求报文放在读缓冲区） 和 **response_**(将响应报文放在写缓冲区)。<br />注意理清这里的读和写的含义，因为请求报文是浏览器发来的，所以需要用读缓冲区去读取，而响应报文是这个连接生成准备发给浏览器的，所以对应写缓冲区<br />解析请求报文和生成响应报文都是在`HttpConn::process()`函数内完成的。并且是在解析请求报文后随即生成了响应报文。之后这个生成的响应报文便放在缓冲区等待`writev()`函数将其发送给fd。
<a name="lNFB8"></a>
### HttpConn的读写
在这段代码里面，还有两个重要的函数，分别是`read`和`write`<br />这里的`read`需要将请求报文的内容读取到读缓冲区里面来，这样request_才能够解析报文。<br />然后是`write`，这里主要是使用了`writev`连续写来将响应报文写到fd（浏览器）中。我们前面提到的生成响应报文，他只是放在了buff中，并没有发送给浏览器，这里用write就实现传输啦。<br />注意由于生成响应报文的响应正文里面只是生成了`Content-length`，并没有将文件也放进缓冲区，因为如果文件太大，缓冲区可能会装不下。所以我们在传输的时候，采用了分块写的方式，一块传输buff里面的内容，另一块传输内存映射的文件指针。<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723295349885-a8ef4f72-c950-4947-b4af-04acbb05e4e3.png#averageHue=%23201f1f&clientId=u38251e1b-1caf-4&from=paste&height=105&id=uf959f3f4&originHeight=174&originWidth=1296&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=30282&status=done&style=none&taskId=u6b53e33f-02e6-4faf-b71f-236f7628782&title=&width=785.454500056498)<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723295269989-94df3783-7048-42eb-b011-23c4c36de3f8.png#averageHue=%23201f1f&clientId=u38251e1b-1caf-4&from=paste&height=421&id=u25452319&originHeight=694&originWidth=1165&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=113181&status=done&style=none&taskId=u5760a532-8cb1-43cc-a5aa-c04804b642a&title=&width=706.0605652514045)
<a name="zC3zP"></a>
### 逻辑代码
```cpp
//只为了说明逻辑，代码有删减
bool HttpConn::process() {
    request_.Init();//初始化解析类
    if(readBuff_.ReadableBytes() <= 0) {//从缓冲区中读数据
        return false;
    }
    else if(request_.parse(readBuff_)) {//解析数据,根据解析结果进行响应类的初始化
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        response_.Init(srcDir, request_.path(), false, 400);
    }
    response_.MakeResponse(writeBuff_);//生成响应报文放入writeBuff_中
    /* 响应头  iov记录了需要把数据从缓冲区发送出去的相关信息
    iov_base为缓冲区首地址，iov_len为缓冲区长度 */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if(response_.FileLen() > 0  && response_.File()) { //
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    return true;
}
```
<a name="RHorr"></a>
# 小根堆实现定时器
<a name="VG107"></a>
## 时间堆
网络编程中除了处理IO事件之外，定时事件也同样不可或缺，如定期检测一个客户连接的活动状态、游戏中的技能冷却倒计时以及其他需要使用超时机制的功能。我们的服务器程序中往往需要处理众多的定时事件，因此有效的组织定时事件，使之能在预期时间内被触发且不影响服务器主要逻辑，对我们的服务器性能影响特别大。<br />我们的web服务器也需要这样一个时间堆，定时剔除掉长时间不动的空闲用户，避免他们占着茅坑不拉屎，耗费服务器资源。<br />一般的做法是将每个定时事件封装成定时器，并使用某种容器类数据结构将所有的定时器保存好，实现对定时事件的统一管理。常用方法有排序链表、红黑树、时间堆和时间轮。这里使用的是**时间堆**。<br />时间堆的底层实现是由小根堆实现的。小根堆可以保证堆顶元素为最小的。<br />![](https://cdn.nlark.com/yuque/0/2024/jpeg/27393008/1723364302555-83643947-98f7-4103-86f0-5443ccb555fc.jpeg#averageHue=%23afa495&clientId=u5ecff4e9-07ab-4&from=paste&id=ud305d3e4&originHeight=2736&originWidth=3648&originalType=url&ratio=1.375&rotation=0&showTitle=false&status=done&style=none&taskId=u578f6305-4b59-4439-b725-7ef3b3ed4df&title=)
<a name="ZSmFe"></a>
## 小根堆详解
传统的定时方案是以固定频率调用起搏函数tick，进而执行定时器上的回调函数。而时间堆的做法则是**将所有定时器中超时时间最小的一个定时器的超时值作为心搏间隔，当超时时间到达时，处理超时事件，然后再次从剩余定时器中找出超时时间最小的一个，依次反复即可**。<br />当前系统时间：8:00<br />1号定时器超时时间：8:05<br />2号定时器超时时间：8:08<br />设置心搏间隔：8:05-8:00=5<br />5分钟到达后处理1号定时器事件，再根据2号超时时间设定心搏间隔.<br />为了后面处理过期连接的方便，我们给每一个定时器里面放置一个回调函数，用来关闭过期连接。<br />为了便于定时器结点的比较，主要是后续堆结构的实现方便，我们还需要重载比较运算符。<br />这里还用到了几个C++11的时间新特性：

- `std::chrono::high_resolution_clock`
   - `duration`（一段时间）
   - `time_point`（时间点）

一般获取时间点是通过clock时钟获得的，一共有3个：<br />①、high_resolution_clock<br />②、system_clock<br />③、steady_clock

- `high_resolution_clock`

他有一个now()方法，可以获取当前时间。注意：std::chrono::high_resolution_clock返回的时间点是按秒为单位的。

- `std::chrono::milliseconds`表示毫秒，可用于`duration<>`的模板类，举例：`chrono::duration_cast<milliseconds>`
```cpp
struct TimerNode{
public:
    int id;             //用来标记定时器
    TimeStamp expire;   //设置过期时间
    TimeoutCallBack cb; //设置一个回调函数用来方便删除定时器时将对应的HTTP连接关闭
    // 重载小于运算符，用于堆的排序
    bool operator<(const TimerNode& t)
    {
        return expire<t.expire;
    }
    // 重载大于运算符，用于堆的排序
    bool operator>(const TimerNode &t){
        return expires > t.expires;
    }
};
```
添加方法：`timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));` 由于TimeoutCallBack的类型是std::function<void()>，所以这里采用bind绑定参数。
<a name="FC2XK"></a>
## 定时器的管理
主要有对堆节点进行增删和调整的操作。
```cpp
void add(int id, int timeOut, const TimeoutCallBack& cb);//添加一个定时器
void del_(size_t i);//删除指定定时器
void siftup_(size_t i);//向上调整
bool siftdown_(size_t index,size_t n);//向下调整,若不能向下则返回false
void swapNode_(size_t i,size_t j);//交换两个结点位置
```
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
## WebServer结构
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
<a name="PQwUF"></a>
# main函数配置启动
main.cpp
```cpp
#include <unistd.h>
#include "server/webserver.h"

int main() {
    /* 守护进程 后台运行 */ 
    //daemon(1,0);

    WebServer server(
        1316, 3, 60000, false,             /* 端口 ET模式 timeoutMs 优雅退出  */
        3306, "ct", "P@ssw0rd2024", "yourdb", /* Mysql配置 */
        12, 6, true, 1, 1024);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.Start();
} 
  
```
这里我们需要配置MYSQL中的用户名，密码和你创建的数据库名（要和你在Linux中创建的对应，不然无法再浏览器注册和登录）<br />Makefile（build中的makefile）
```cpp
CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = server2
OBJS = ../code/log/*.cpp ../code/pool/*.cpp ../code/timer/*.cpp \
       ../code/http/*.cpp ../code/server/*.cpp \
       ../code/buffer/*.cpp ../code/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)
```
<a name="qxKkJ"></a>
# 压力测试
更多内容可以参考：[WebServer 跑通/运行/测试（详解版）_webserver测试-CSDN博客](https://blog.csdn.net/csdner250/article/details/135503751?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522172343072816800227442681%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=172343072816800227442681&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-1-135503751-null-null.142^v100^pc_search_result_base5&utm_term=WebServer%E5%8E%8B%E5%8A%9B%E6%B5%8B%E8%AF%95&spm=1018.2226.3001.4187)<br />我们需要先安装依赖
```bash
sudo apt-get install exuberant-ctags
```
然后cd 到webbench的目录，直接make<br />然后我们可以进行测试：
```bash
webbench -c 500 -t 30 http://baidu.com/
```
![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723432401890-d6e94acb-d6c2-4669-889f-49dce25616be.png#averageHue=%23222120&clientId=u3d411463-ecdc-4&from=paste&id=ue7dc4bdd&originHeight=221&originWidth=1015&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=25969&status=done&style=none&taskId=ua5d244da-c487-4054-83f6-91fc99278a8&title=)<br />可以看到可以成功运行，接下来我们先把自己的服务器跑起来
```bash
./bin/server2
```
![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723440262764-04c64a41-0262-469d-a89b-d0596b38c178.png#averageHue=%231f1f1e&clientId=ue84fba2c-5908-4&from=paste&height=60&id=u0128f9d9&originHeight=99&originWidth=654&originalType=binary&ratio=1.6500000953674316&rotation=0&showTitle=false&size=5466&status=done&style=none&taskId=ue3aefa3f-29d5-4338-8263-fe7f138224f&title=&width=396.36361345443646)<br />再用这个来测试
```bash
webbench -c 20000 -t 10 http://192.168.92.133:1316/
```
可以看到我们很轻松就实现了20000并发<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723440288905-4b47393d-6ce8-4186-91c0-75d234bf8ac9.png#averageHue=%23212020&clientId=ue84fba2c-5908-4&from=paste&height=165&id=u007b6c14&originHeight=273&originWidth=1047&originalType=binary&ratio=1.6500000953674316&rotation=0&showTitle=false&size=30366&status=done&style=none&taskId=u8158acaf-1870-4b06-bf89-d0b8fb6d009&title=&width=634.5454178697171)<br />内存：8G，CPU：R9-5900H ,Ubuntu:18.04
> 注意：如果内存只有4G，QPS差不多在10000左右，因为虚拟机的性能会限制进程数量



