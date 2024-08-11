#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //包含 epoll_ctl() 函数的定义
#include <fcntl.h>  // 包含 fcntl() 函数的定义
#include <unistd.h> 
#include <assert.h> 
#include <vector>
#include <errno.h>

using namespace std;

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    bool AddFd(int fd, uint32_t events);    //添加文件描述符到epoll实例，并指定其感兴趣的事件
    bool ModFd(int fd, uint32_t events);    //修改一个文件描述符感兴趣的事件
    bool DelFd(int fd);     //从epoll实例中删除一个文件描述符
    int Wait(int timeoutMs = -1);   //无限等待事件发生
    int GetEventFd(size_t i) const;     //获取第i个事件的文件描述符
    uint32_t GetEvents(size_t i) const;     //获取第i个事件的类型
        
private:
    int epollFd_;   //epoll实例的文件描述符

    vector<struct epoll_event> events_;    //存储epoll事件的数组
};

#endif //EPOLLER_H