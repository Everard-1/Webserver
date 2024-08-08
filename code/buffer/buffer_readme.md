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
