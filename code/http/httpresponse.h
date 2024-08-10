#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // 提供文件控制操作，如open
#include <unistd.h>      // 提供对POSIX操作系统API的访问，如close
#include <sys/stat.h>    // 提供文件状态信息的头文件，stat
#include <sys/mman.h>    // 提供内存映射文件的头文件，mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

using namespace std;

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    // 初始化 HTTP 响应对象
    // srcDir: 静态文件的根目录
    // path: 请求的路径
    // isKeepAlive: 是否保持连接
    // code: HTTP 状态码
    void Init(const string& srcDir, string& path, bool isKeepAlive = false, int code = -1);   

    void MakeResponse(Buffer& buff); // 生成HTTP响应
    void UnmapFile();   // 解除内存映射文件 
    char* File();    // 返回文件内容的指针
    size_t FileLen() const; // 返回文件长度
    void ErrorContent(Buffer& buff, string message);    // 设置错误内容
    int Code() const { return code_; }  // 获取 HTTP 状态码

private:
    void AddStateLine_(Buffer &buff);   //添加状态行（例如，HTTP/1.1 200 OK）
    void AddHeader_(Buffer &buff);  //添加http头部（例如 Content-Type、Content-Length）
    void AddContent_(Buffer &buff); //添加响应内容（文件或错误页面）

    void ErrorHtml_();  //生成错误页面的 HTML内容
    string GetFileType_();  //获取文件类型（根据后缀)

    int code_;              // HTTP 状态码
    bool isKeepAlive_;     // 是否保持连接

    string path_;   //请求路径
    string srcDir_; //讲台文件根目录
    
    char* mmFile_;  //内存文件映射指针
    struct stat mmFileStat_;    //文件状态信息

    static const unordered_map<string, string> SUFFIX_TYPE;     // 文件状态与MIME类型的映射
    static const unordered_map<int, string> CODE_STATUS;    //状态码与状态描述的映射
    static const unordered_map<int, string> CODE_PATH;      //状态码与错误页面路径的映射
};


#endif //HTTP_RESPONSE_H