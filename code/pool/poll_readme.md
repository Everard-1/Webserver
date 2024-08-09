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
