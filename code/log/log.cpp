 #include "log.h"

using namespace std;

// 构造函数
Log::Log() {
    fp_ = nullptr;
    deque_ = nullptr;
    writeThread_ = nullptr;
    lineCount_ = 0;
    toDay_ = 0;
    isAsync_ = false;
}

// 析构函数
Log::~Log() {
    if(writeThread_ && writeThread_->joinable()) {  //检查 writeThread_ 是否存在且是否可联接
        //队非空，唤醒消费者
        while(!deque_->empty()) {
            deque_->flush();
        }
        deque_->Close();
        writeThread_->join();   //等待线程
    }
    //冲洗文件缓冲区，关闭文件描述符
    if(fp_) {
        lock_guard<mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

//异步日志，刷新队列
void Log::flush() {
    if(isAsync_) { 
        deque_->flush(); 
    }
    fflush(fp_);    //刷新输出缓冲区，立刻写入文件
}

// 懒汉模式 局部静态变量法（C++11后不需要加锁和解锁）
Log* Log::Instance() {
    static Log inst;    //只在第一次调用创建并初始化，后续使用同一实例
    return &inst;
}

// 异步日志的写线程函数
void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}

// 真正的写线程函数
void Log::AsyncWrite_() {
    string str = "";
    while(deque_->pop(str)) {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);    //将C风格字符串 str 写入到日志文件中
    }
}

//初始化日志实例
void Log::init(int level = 1, const char* path, const char* suffix,int maxQueueSize) {
    isOpen_ = true;     //标记日志系统为打开状态
    level_ = level;
    if(maxQueueSize > 0) {
        isAsync_ = true;    //启用异步日志模式
        if(!deque_) {   // 队列为空就创建一个
            unique_ptr<BlockDeque<string>> newDeque(new BlockDeque<string>);
            // 因为unique_ptr不支持普通的拷贝或赋值操作,所以采用move
            // 将动态申请的内存权给deque，newDeque被释放
            deque_ = move(newDeque);    // 左值变右值,掏空newDeque
            
            //创建新线程写入日志
            unique_ptr<thread> NewThread(new thread(FlushLogThread));
            writeThread_ = move(NewThread);
        }
    } 
    else {
        isAsync_ = false;
    }

    lineCount_ = 0;

    time_t timer = time(nullptr);   //获取当前时间
    struct tm *sysTime = localtime(&timer); //将当前时间转换为本地时间
    struct tm t = *sysTime;

    // 设置日志文件路径和文件名后缀
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    // 格式化并生成日志文件名。文件名包括路径、日期和后缀
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = sysTime->tm_mday;  //等于t.tm_mday也可以

    //打开日志文件——创建局部作用域
    {
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();    //清空缓冲区中的数据
        // 如果日志文件指针 fp_ 已经存在，先冲洗和关闭它
        if(fp_) { 
            flush();
            fclose(fp_); 
        }

        fp_ = fopen(fileName, "a"); //以追加模式打开新的日志文件
        // 打开失败，则创建目录并重新尝试打开文件
        if(fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a"); // 生成目录文件（最大权限）
        } 
        assert(fp_ != nullptr); //确保文件成功打开
    }
}

// 写入日志、日志轮转、同步/异步写入
void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);    //获取当前时间
    time_t tSec = now.tv_sec;   
    struct tm *sysTime = localtime(&tSec);  //将时间戳转换为本地时间
    struct tm t = *sysTime;
    va_list vaList;     //处理可变参数列表

    /* 日志日期 日志行数 如果不是今天或行数刚好为最大行数整数倍——轮转日志文件*/
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_  %  MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(mtx_);
        locker.unlock();
        
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        // 日期发生变化，则使用新日期的文件名
        if (toDay_ != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else {  //使用轮转次数后缀
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / MAX_LINES), suffix_);
        }
        
        //文件轮转
        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    // 在buffer内生成一条对应的日志信息
    {
        unique_lock<mutex> locker(mtx_);
        lineCount_++;
        // 格式化时间戳，并写入缓冲区 buff_
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);    //添加日志级别标题

        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);   //格式化可变参数并写入缓冲区
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);    //添加换行符到日志消息末尾

        //启用了异步日志且日志队列未满，将日志消息添加到队列 deque_ 
        if(isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.RetrieveAllToStr());
        } else {    //同步日志，将日志直接写入当前文件 fp_
            fputs(buff_.Peek(), fp_);
        }
        buff_.RetrieveAll();     // 清空buff
    }
}

// 添加日志等级
void Log::AppendLogLevelTitle_(int level) {
    switch(level) {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

// 获取当前日志级别
int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

// 设置当前日志级别
void Log::SetLevel(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}