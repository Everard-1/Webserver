#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <assert.h>

using namespace std;

class ThreadPool {
public:
    // 尽量使用make_shared代替new，因为通过new传递给shared_ptr的内存是不连续的，会造成内存碎片连续化
    explicit ThreadPool(size_t threadCount = 8): pool_(make_shared<Pool>()) {   //make_shared：传递右值，功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
            assert(threadCount > 0);    // 确保线程池中有线程
            // 创建 threadCount 个线程，每个线程执行一个任务
            for(size_t i = 0; i < threadCount; i++) {
                thread([pool = pool_] {
                    unique_lock<std::mutex> locker(pool->mtx_);
                    while(true) {
                        if(!pool->tasks.empty()) {
                            auto task = move(pool->tasks.front()); //左值变右值，资产转移
                            pool->tasks.pop();
                            locker.unlock();    //把任务取出来了，可以提前解锁
                            task();
                            locker.lock();  //马上又要取任务了，直接上锁
                        } 
                        else if(pool->isClosed) 
                            break;
                        else 
                            pool->cond_.wait(locker);    //等待，条件来了就唤醒notify
                    }
                }).detach();    //线程分离，在后台运行不阻塞主线程
            }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {  //检查线程池是否有效
            {
                lock_guard<mutex> locker(pool_->mtx_);
                pool_->isClosed = true;
            }
            pool_->cond_.notify_all();   // 唤醒所有的线程
        }
    }

    // 添加新任务
    template<class F>
    void AddTask(F&& task) {
        {
            lock_guard<mutex> locker(pool_->mtx_);
            pool_->tasks.emplace(forward<F>(task)); // 以完美转发方式添加到队列中
        }
        pool_->cond_.notify_one();  //唤醒一个线程并处理这个任务
    }

private:
    // 线程池结构体
    struct Pool {
        mutex mtx_;
        condition_variable cond_;
        bool isClosed;
        queue<function<void()>> tasks;  //任务队列，函数类型为void
    };
    shared_ptr<Pool> pool_;
};

#endif //THREADPOOL_H