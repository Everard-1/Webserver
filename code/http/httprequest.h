#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>    //正则表达式
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

using namespace std;

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    enum PARSE_STATE {
        REQUEST_LINE,  // 解析请求行
        HEADERS,       // 解析请求头
        BODY,          // 解析请求体
        FINISH,        // 解析完成
    };

enum HTTP_CODE {
    NO_REQUEST = 0,           // 没有请求，服务器未收到请求或请求无效
    GET_REQUEST,              // GET 请求，客户端请求资源
    BAD_REQUEST,              // 错误请求，请求格式不正确或无法解析
    NO_RESOURSE,              // 资源不存在，服务器无法找到请求的资源
    FORBIDDENT_REQUEST,       // 禁止请求，客户端没有权限访问请求的资源
    FILE_REQUEST,             // 文件请求，服务器请求文件资源
    INTERNAL_ERROR,           // 内部错误，服务器遇到意外情况无法处理请求
    CLOSED_CONNECTION,        // 连接关闭，连接被客户端或服务器关闭
};

    void Init();             // 初始化请求
    bool parse(Buffer& buff); // 解析请求内容

    string path() const;    // 获取请求路径（只读）
    string& path();         // 获取请求路径（可读写）
    string method() const;  // 获取请求方法
    string version() const; // 获取 HTTP 版本
    string GetPost(const string& key) const; // 获取 POST 请求中的数据（通过键）
    string GetPost(const char* key) const; // 获取 POST 请求中的数据（通过 C 字符串键）

    bool IsKeepAlive() const; // 判断是否保持连接

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    bool ParseRequestLine_(const string& line); // 解析请求行
    void ParseHeader_(const string& line);      // 解析请求头
    void ParseBody_(const string& line);        // 解析请求体

    void ParsePath_();                               // 处理请求路径
    void ParsePost_();                               // 处理 POST 请求
    void ParseFromUrlencoded_();                     // 从 URL 编码中解析数据

    static bool UserVerify(const string& name, const string& pwd, bool isLogin); // 用户验证

    PARSE_STATE state_; // 当前解析状态
    string method_, path_, version_, body_; // 请求方法、路径、版本和体
    unordered_map<string, string> header_; // 请求头
    unordered_map<string, string> post_;    // POST 数据

    static const unordered_set<string> DEFAULT_HTML; // 默认 HTML 页面
    static const unordered_map<string, int> DEFAULT_HTML_TAG; // 默认 HTML 标签
    static int ConverHex(char ch); // 将 16 进制字符转换为 10 进制整数
};


#endif //HTTP_REQUEST_H