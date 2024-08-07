#include "buffer.h"

// 读、写下标初始化，vector<char>初始化
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

// 可读的数量：写下标 - 读下标
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// 可写的数量：buffer大小 - 写下标
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 可预留空间：已经读过的就没用了，等于读下标
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 返回缓冲区当前读取位置指针
const char* Buffer::Peek() const {
    //如果涉及特定的缓冲区管理逻辑
    //return BeginPtr_() + readPos_;

    //如果buffer是一个简单数组
    return &buffer_[readPos_];
}

// 确保最少可写长度
void Buffer::EnsureWriteable(size_t len) {
    if(WritableBytes() < len) {
        MakeSpace_(len);
    }
    //确保空间足够
    assert(WritableBytes() >= len);
}

// 移动写下标，在Append中使用
void Buffer::HasWritten(size_t len) {
    writePos_ += len;   //可写不够会自动扩展
} 

// 读取len长度，移动读下标
void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

// 读取到end位置
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek()); //剩下的长度 = end指针 - 读指针
}

// 取出所有数据，buffer归零，读写下标归零,在别的函数中会用到
void Buffer::RetrieveAll() {
    //bzero 是一个较旧的函数，在 POSIX 标准中已经被标记为不推荐使用
    //bzero(&buffer_[0], buffer_.size());// 覆盖原本数据

    //使用memset替代
    memset(&buffer_[0],0,buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

// 取出剩余可读的str
std::string Buffer::RetrieveAllToStr() {
    //C++14以上语法：std::string
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

// 写指针的位置(不可写操作)
const char* Buffer::BeginWriteConst() const {
    //return BeginPtr_() + writePos_;

    //简化
    return &buffer_[writePos_];
}

// 写指针位置（可写操作）
char* Buffer::BeginWrite() {
    //return BeginPtr_() + writePos_;
    return &buffer_[writePos_];
}

// 底层：将 len 字节的 str 内容追加到缓冲区中
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);   // 确保可写的长度
    std::copy(str, str + len, BeginWrite());    // 将str放到写下标开始的地方 
    HasWritten(len);    // 更新写下标
}

// 将 std::string 对象的内容追加到缓冲区中
void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

// 将 void* 类型的数据追加到缓冲区中
void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

// 将buffer中的读下标的地方放到该buffer中的写下标位置
void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 将fd的内容读到缓冲区，即writable的位置
ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char buff[65535];   // 栈区
    struct iovec iov[2];    //分散读
    const size_t writable = WritableBytes();    //可写大小
    //分散读， 保证数据全部读完 
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);  //读取数据到buffer和栈区
    if(len < 0) {
        *Errno = errno; 
    }
    else if(static_cast<size_t>(len) <= writable) {    // 若len小于writable，说明写区可以容纳len
        writePos_ += len;   //更新写下标
    }
    else {  
        writePos_ = buffer_.size(); // 写区写满了,下标移到最后
        Append(buff, len - writable);   //超出可写数据写入栈中
    }
    return len;
}

// 将buffer中可读的区域写入fd中
ssize_t Buffer::WriteFd(int fd, int* Errno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *Errno = errno;
        return len;
    } 
    readPos_ += len;
    return len;
}

//----------------------私有成员函数------------------------------------

// 返回缓冲区的起始指针（允许修改数据）
char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

// 返回缓冲区的起始指针（不允许修改数据）
const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

//扩展空间
void Buffer::MakeSpace_(size_t len) {
    //如果可扩展空间不够，扩大buffer
    if(WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } 
    else {
        size_t readable = ReadableBytes();
        //把可读数据移动到缓冲区开始位置
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        //更新可读可写指针位置
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        //确保 `readable` 等于 `ReadableBytes()`，以验证数据移动正确
        assert(readable == ReadableBytes());
    }
}