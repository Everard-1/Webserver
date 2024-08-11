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
