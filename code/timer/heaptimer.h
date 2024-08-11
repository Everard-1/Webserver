#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

using namespace std;

typedef function<void()> TimeoutCallBack;  //定义超时回调函数类型
typedef chrono::high_resolution_clock Clock;    //定义高精度时钟类型
typedef chrono::milliseconds MS;    //定义毫秒时间单位类型
typedef Clock::time_point TimeStamp;    //定义时间点类型

// 定时器节点结构体
struct TimerNode {
    int id; //定时器标识符
    TimeStamp expires;  //超时时间点
    TimeoutCallBack cb; //超时回调函数

    // 重载小于运算符，用于堆的排序
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }

    // 重载大于运算符，用于堆的排序
    bool operator>(const TimerNode &t){
        return expires > t.expires;
    }
};

// 定时器类
class HeapTimer {
public:
    HeapTimer();
    ~HeapTimer();
    
    void adjust(int id, int timeOut);    //调整定时器超时时间

    void add(int id, int timeOut, const TimeoutCallBack& cb);   //添加定时器

    void doWork(int id);    //执行定时器的回调函数

    void clear();   //清理所有定时器

    void tick();     //执行到期的定时器

    void pop();     //弹出堆顶定时器

    int GetNextTick();  //获取下一个定时器的超时时间（单位：毫秒）

private:
    void del_(size_t i);    //删除堆中指定位置的节点
    
    void siftup_(size_t i);     //上浮操作，维护小根堆

    bool siftdown_(size_t index, size_t n);     //下沉操作，维护小根堆

    void SwapNode_(size_t i, size_t j);     //交换两个节点

    vector<TimerNode> heap_;    //定时器堆，存储定时器节点

    unordered_map<int, size_t> ref_;    //id到堆中节点下标的映射，方便查找和修改
};

#endif //HEAP_TIMER_H