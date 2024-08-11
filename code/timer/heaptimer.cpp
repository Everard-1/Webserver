#include "heaptimer.h"
using namespace std;

HeapTimer::HeapTimer() { 
    heap_.reserve(64);  //初始化堆，保留64个容量
}

HeapTimer::~HeapTimer() { 
    clear();    //清理堆中所有的定时器
}

// 交换堆中两个节点的位置，并更新它们在映射表中的位置
void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    swap(heap_[i], heap_[j]);       // 交换堆中两个节点
    ref_[heap_[i].id] = i;          // 更新节点i在堆中的位置
    ref_[heap_[j].id] = j;          // 更新节点j在堆中的位置
}

// 上浮操作
void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t parent = (i - 1) / 2;     //父节点的索引
    while(parent >= 0) {
        //如果当前结点小于父节点，交换并更新索引
        if(heap_[i]<heap_[parent]){
            SwapNode_(i,parent);
            i=parent;
            parent=(i-1)/2;
        }
        else
            break;
    }
}

// 下沉操作
bool HeapTimer::siftdown_(size_t i, size_t n) {
    assert(i >= 0 && i < heap_.size());
    assert(n >= 0 && n <= heap_.size());    //n:总结点数
    size_t index = i;
    size_t child = index * 2 + 1;   //左子节点的索引
    while(child < n) {
        if(child + 1 < n && heap_[child + 1] < heap_[child]){
            child++;    //如果右子节点存在且小于左子结点，选择右子节点
        }
        // 如果只有左孩子且小于父节点，父节点下沉并更新左孩子
        if(heap_[child]<heap_[index]){
            SwapNode_(child,index);
            index=child;    // 更新当前节点为子节点
            child=2*child+1;
        }
        else{
            break;  //如果当前结点小于等于子节点，无需下沉
        }
    }
    return index>i;   // 如果当前节点的索引发生了变化，说明需要下沉
}

// 调整指定id的节点的超时时间
void HeapTimer::adjust(int id, int timeout) {
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);  //更新超时时间点
    siftdown_(ref_[id], heap_.size());   // 进行下沉操作以维持堆的性质
}

// 添加一个新的定时器
void HeapTimer::add(int id, int timeOut, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t tmp;
    if(ref_.count(id)){ //如果定时器已存在
        tmp=ref_[id];
        heap_[tmp].expires=Clock::now()+MS(timeOut);    //更新超时时间
        heap_[tmp].cb=cb;   //更新回调函数
        if(!siftdown_(tmp,heap_.size())){
            siftup_(tmp);
        }
    }
    else{   //如果定时器不存在
        tmp=heap_.size();   //获取最后一个节点的下一个节点的索引
        ref_[id]=tmp;   //更新定时器为“最后一个节点”
        heap_.push_back({id,Clock::now()+MS(timeOut),cb});  //在最后一个节点添加新的定时器
        siftup_(tmp);
    }
}

// 删除制定id的定时器，并触发其回调函数
void HeapTimer::doWork(int id) {
    if(heap_.empty() || ref_.count(id) == 0) {
        return; // 如果堆为空或者指定id不存在，则返回
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();  //触发回调函数
    del_(i);    //删除指定id的定时器
}

// 删除指定位置的节点
void HeapTimer::del_(size_t index) {
    assert(!heap_.empty() && index >= 0 && index < heap_.size());   //确保堆不为空且参数正确
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t tmp = index;
    size_t n = heap_.size() - 1;    // 堆的最后一个节点索引
    assert(tmp <= n);
    // 如果删除节点没有变成最后一个节点，一直调整
    if(tmp < n) {
        SwapNode_(tmp, n);  //交换删除的节点和堆的最后一个节点
        if(!siftdown_(tmp, n)) {
            siftup_(tmp);   //下沉操作未成功，就上浮
        }
    }
    ref_.erase(heap_.back().id);    // 从映射表中删除节点的id
    heap_.pop_back();      // 从堆中删除最后一个节点
}

// 执行所有超时的定时器
void HeapTimer::tick() {
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) {
        TimerNode node = heap_.front(); //获取堆顶（最早到期）的定时器
        if(chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { 
            break;  //如果定时器尚未到期，则跳出循环
        }
        node.cb();  //触发回调函数
        pop();  //删除堆顶定时器
    }
}

// 删除堆顶定时器
void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);    //删除堆顶节点
}

// 清除所有定时器
void HeapTimer::clear() {
    ref_.clear();   //清除映射表
    heap_.clear();  //清除堆
}

// 获取获取下一个到期定时器的超时时间（毫秒）
int HeapTimer::GetNextTick() {
    tick(); //执行所有超时的定时器
    size_t res = -1;
    if(!heap_.empty()) {
        res = chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(res < 0) { res = 0; }    // 如果超时时间为负，则将其设置为0
    }
    return res;
}