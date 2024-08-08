#include "blockqueue.h"
using namespace std;

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) :capacity_(MaxCapacity) {
    assert(MaxCapacity > 0);    // 确保最大容量为正值
    isClose_ = false;   // 开启初始化队列
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();    //析构函数关闭队列
};

// 关闭队列函数
template<class T>
void BlockDeque<T>::Close() {
    clear();
    isClose_=true;  // 关闭初始化队列
    condProducer_.notify_all(); // 唤醒所有等待中的生产者
    condConsumer_.notify_all(); // 唤醒所有等待中的消费者
};

// 清空队列函数
template<class T>
void BlockDeque<T>::clear() {
    lock_guard<mutex> locker(mtx_); // 上锁，保护队列操作
    deq_.clear();                   // 清空队列
}

// 判断队列是否为空
template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

// 判断队列是否满
template<class T>
bool BlockDeque<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

// 向队列末尾添加一个元素
template<class T>
void BlockDeque<T>::push_back(const T &item) {
    unique_lock<mutex> locker(mtx_);
    // 队满，生产者阻塞
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_back(item);   // 将元素添加到队列末尾
    condConsumer_.notify_one(); // 唤醒一个等待中的消费者
}

// 向队列头部添加一个元素
template<class T>
void BlockDeque<T>::push_front(const T &item) {
    unique_lock<mutex> locker(mtx_);
    // 队满，生产者阻塞
    while(deq_.size() >= capacity_) {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);  // 将元素添加到队列头部
    condConsumer_.notify_one(); // 唤醒一个等待中的消费者
}

// 从队列头部移除一个元素，并将其赋值给 item
template<class T>
bool BlockDeque<T>::pop(T &item) {
    unique_lock<mutex> locker(mtx_);
    // 队空，消费者阻塞
    while(deq_.empty()){
        condConsumer_.wait(locker);
        if(isClose_){   // 队列关闭
            return false;
        }
    }
    item = deq_.front();  // 获取队列头部的元素
    deq_.pop_front();  // 移除队列头部的元素
    condProducer_.notify_one();  // 唤醒一个等待中的生产者
    return true;
}

// 从队列头部移除一个元素，并将其赋值给 item，但带有超时机制
template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    unique_lock<mutex> locker(mtx_);
    while(deq_.empty()){
        // 等待超时
        if(condConsumer_.wait_for(locker, chrono::seconds(timeout)) 
                == cv_status::timeout){
            return false;
        }
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();  // 获取队列头部的元素
    deq_.pop_front();  // 移除队列头部的元素
    condProducer_.notify_one();  // 唤醒一个等待中的生产者
    return true;
}

// 返回队头元素
template<class T>
T BlockDeque<T>::front() {
    lock_guard<mutex> locker(mtx_);
    return deq_.front();
}

// 返回队尾元素
template<class T>
T BlockDeque<T>::back() {
    lock_guard<mutex> locker(mtx_);
    return deq_.back();
}

//返回队列最大容量
template<class T>
size_t BlockDeque<T>::capacity() {
    lock_guard<mutex> locker(mtx_);
    return capacity_;
}

// 返回队列当前大小
template<class T>
size_t BlockDeque<T>::size() {
    lock_guard<mutex> locker(mtx_);
    return deq_.size();
}

// 唤醒一个等待中的消费者
template<class T>
void BlockDeque<T>::flush() {
    condConsumer_.notify_one();
};