#include "epoller.h"

// 构造函数：初始化epoll实例，并设置事件数组的大小
Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)),events_(maxEvent){
    assert(epollFd_ >= 0 && events_.size() > 0);    //确保epoll文件描述符有效，且事件数组大小大于0
}

// 析构函数：关闭epoll实例
Epoller::~Epoller() {
    close(epollFd_);    //关闭epoll文件描述符
}

// 添加文件描述符到 epoll 实例中，并设置其感兴趣的事件
bool Epoller::AddFd(int fd, uint32_t events) {
    if(fd < 0) 
        return false;
    epoll_event ev = {0};   //创建并初始化epoll_event结构体
    ev.data.fd = fd;    
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);     // 调用 epoll_ctl 添加文件描述符
}

// 修改文件描述符的感兴趣事件
bool Epoller::ModFd(int fd, uint32_t events) {
    if(fd < 0) 
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);    // 调用 epoll_ctl 修改文件描述符
}

// 从epoll实例中删除文件描述符
bool Epoller::DelFd(int fd) {
    if(fd < 0) 
        return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);     // 调用 epoll_ctl 删除文件描述符
}

// 等待事件发生，并返回发生事件的数量
int Epoller::Wait(int timeoutMs) {
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);  // timeoutMs 指定超时时间，单位为毫秒，-1 表示无限等待
}

// 获取第i个事件的文件描述符
int Epoller::GetEventFd(size_t i) const {
    assert(i < events_.size() && i >= 0);  // 确保索引有效
    return events_[i].data.fd;  // 返回第 i 个事件的文件描述符
}

// 获取第 i 个事件的事件类型
uint32_t Epoller::GetEvents(size_t i) const {
    assert(i < events_.size() && i >= 0);  // 确保索引有效
    return events_[i].events;  // 返回第 i 个事件的事件类型
}