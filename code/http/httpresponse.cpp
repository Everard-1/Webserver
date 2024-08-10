#include "httpresponse.h"

using namespace std;

// 初始化响应报文
HttpResponse::HttpResponse() {
    code_ = -1;                // 初始状态码为 -1，表示未设置
    path_ = srcDir_ = "";     // 路径和源目录初始化为空字符串
    isKeepAlive_ = false;    // 默认不保持连接
    mmFile_ = nullptr;       // 内存映射文件指针初始化为空
    mmFileStat_ = { 0 };     // 文件状态信息初始化
};

// 确保在销毁对象时解除文件映射
HttpResponse::~HttpResponse() {
    UnmapFile();    
}

// 解除文件映射
void HttpResponse::UnmapFile() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);   //解除内存映射
        mmFile_ = nullptr;
    }
}

// 文件后缀与 MIME 类型的映射
const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

// HTTP 状态码与状态描述的映射
const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

// HTTP 状态码与错误页面路径的映射
const unordered_map<int, string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

// 初始化 HTTP 响应
void HttpResponse::Init(const string& srcDir, string& path, bool isKeepAlive, int code){
    assert(srcDir != "");
    if(mmFile_) { UnmapFile(); }    //如果映射了文件，先解除映射
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr;      //重新初始化内存文件映射指针
    mmFileStat_ = { 0 };    //重新初始化文件状态信息
}

// 生成 HTTP 响应
void HttpResponse::MakeResponse(Buffer& buff) {
    //判断请求的资源文件 
    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;    // 文件不存在或路径是目录，返回 404
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;    // 文件不可读，返回403
    }
    else if(code_ == -1) { 
        code_ = 200;    // 状态码未设置，默认为200
    }
    ErrorHtml_();          // 设置错误页面内容（如果需要）
    AddStateLine_(buff);  // 添加状态行
    AddHeader_(buff);     // 添加响应头
    AddContent_(buff);    // 添加响应内容
}

// 返回内存映射文件的指针
char* HttpResponse::File() {
    return mmFile_;
}

// 返回文件长度
size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

// 错误页面HTML内容
void HttpResponse::ErrorHtml_() {
    //如果状态码有对应的错误页面
    if(CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;  //设置路径为对应的错误页面
        stat((srcDir_ + path_).data(), &mmFileStat_);   //获取错误
    }
}

// 添加状态行（例如，HTTP/1.1 200 OK）
void HttpResponse::AddStateLine_(Buffer& buff) {
    string status;
    if(CODE_STATUS.count(code_) == 1) { // 如果状态码有对应的状态描述
        status = CODE_STATUS.find(code_)->second;   //对应的状态消息
    }
    else {
        code_ = 400;    //默认状态码400
        status = CODE_STATUS.find(400)->second; 
    }
    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

// 添加 HTTP 头部（例如 Content-Type、Content-Length）
void HttpResponse::AddHeader_(Buffer& buff) {
    buff.Append("Connection: ");
    if(isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } 
    else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

// 添加响应内容（文件或错误页面）
void HttpResponse::AddContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);   //打开文件
    if(srcFd < 0) { 
        ErrorContent(buff, "File NotFound!");   //文件打开失败
        return; 
    }

    /* 将文件映射到内存，提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");   //映射失败，返回错误内容
        return; 
    }
    mmFile_ = (char*)mmRet; // 设置内存映射文件指针
    close(srcFd);   //关闭文件描述符
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

// 获取文件类型
string HttpResponse::GetFileType_() {
    /* 判断文件类型 */
    string::size_type idx = path_.find_last_of('.');    //查找最后一个 . 的位置
    if(idx == string::npos) {   //如果没有找到
        return "text/plain";
    }
    string suffix = path_.substr(idx);  //获取文件后缀
    if(SUFFIX_TYPE.count(suffix) == 1) {    //如果后缀有对应的MIME类型
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";  
}

// 设置错误内容
void HttpResponse::ErrorContent(Buffer& buff, string message) 
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } 
    else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");  //添加内容长度
    buff.Append(body);  //添加错误页面内容
}
