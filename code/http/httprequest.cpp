#include "httprequest.h"
using namespace std;

HttpRequest::HttpRequest(){
    Init();
}

HttpRequest::~HttpRequest(){

}

// 默认的 HTML 页面路径
const unordered_set<string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

// 存储 HTML 页面路径及其对应的标签
const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

// 初始化http请求对象
void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;  //初始化状态为——解析请求行
    header_.clear();
    post_.clear();
}

// 是否保持连接 ----头部为“keep-alive”，http版本为1.1即保持
bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

// 解析http请求
bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n"; // 定义CRLF常量，作为行结束标志
    if(buff.ReadableBytes() <= 0) {
        return false;   // 如果缓冲区没有可读字节，返回false
    }
    // 循环直到状态为FINISH或缓冲区没有更多数据
    while(buff.ReadableBytes() && state_ != FINISH) {
        // 查找当前缓冲区中CRLF的位置
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        // 转化为string类型
        string line(buff.Peek(), lineEnd);   //获取当前行

        switch(state_)
        {
        /*
            有限状态机，从请求行开始，每处理完后会自动转入到下一个状态    
        */
        case REQUEST_LINE:
            // 解析请求行
            if(!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_();   //解析路径
            break;    
        case HEADERS:
            // 解析请求头
            ParseHeader_(line);
            // 如果缓冲区剩余字节数小于等于2，表示头部部分结束
            if(buff.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            // 解析请求体
            ParseBody_(line);
            break;
        default:
            break;
        }

        // 如果数据已经读完
        if(lineEnd == buff.BeginWrite()) { break; }
        // 跳过回车换行符
        buff.RetrieveUntil(lineEnd + 2);
    }
    // 返回请求日志
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

// 解析请求路径
void HttpRequest::ParsePath_() {
    if(path_ == "/") {
        path_ = "/index.html";  // 默认路径映射到index.html
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";   // 如果路径在默认HTML列表中，添加.html后缀
                break;
            }
        }
    }
}

// 解析请求行
bool HttpRequest::ParseRequestLine_(const string& line) {
    // 在匹配规则中，以括号()的方式来划分组别 一共三个括号 [0]表示整体
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");  // 请求行的正则表达式
    smatch subMatch;
    
    if(regex_match(line, subMatch, patten)) {   // 匹配指定字符串整体是否符合
        method_ = subMatch[1];  // 请求方法
        path_ = subMatch[2];    // 请求路径
        version_ = subMatch[3]; // HTTP版本
        state_ = HEADERS;       // 解析完成后进入头部解析状态
        return true;
    }
    // 请求行解析失败，返回false
    LOG_ERROR("RequestLine Error");
    return false;  
}

// 解析请求头部
void HttpRequest::ParseHeader_(const string& line) {
    regex patten("^([^:]*): ?(.*)$");  // 请求头部的正则表达式
    smatch subMatch;
    
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];  // 将头部字段及其值存储到header_中
    }
    else {
        state_ = BODY;  // 如果解析失败，进入请求体解析状态
    }
}

// 解析请求体
void HttpRequest::ParseBody_(const string& line) {
    body_ = line;  // 直接将请求体存储到body_
    ParsePost_();  // 解析POST请求数据
    state_ = FINISH;  // 解析完成，状态设为FINISH
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());//解析完成，添加日志
}

// 解析POST请求数据
void HttpRequest::ParsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {  
        ParseFromUrlencoded_();  // 解析urlencoded格式的数据
        if(DEFAULT_HTML_TAG.count(path_)) { // 如果是登录/注册的path
            int tag = DEFAULT_HTML_TAG.find(path_)->second; //获取与请求路径相关联的标签值
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);  // 为1则是登录
                if(UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";  // 登录或注册成功，路径设置为欢迎页面
                } 
                else {
                    path_ = "/error.html";  // 登录或注册失败，路径设置为错误页面
                }
            }
        }
    }   
}

// 解析URL编码的POST数据
void HttpRequest::ParseFromUrlencoded_() {
    if(body_.size() == 0) { return; }  // 如果请求体为空，则直接返回

    string key, value;
    int num = 0;    
    int n = body_.size();
    int i = 0, j = 0;

    // 遍历请求体中的每个字符
    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        // key
        case '=':
            key = body_.substr(j, i - j);  // 提取键
            j = i + 1;  // 更新键值对的开始位置
            break;
        case '+':
            // URL 编码中，空格用 '+' 代替
            body_[i] = ' ';  // 替换空格符
            break;
        case '%':
            // URL 编码中，%后面是十六进制数，表示一个字符
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);  // 转换为十进制数值
            body_[i + 2] = num % 10 + '0';  // 更新当前字符为转换后的数字
            body_[i + 1] = num / 10 + '0';  // 更新下一个字符为转换后的数字
            i += 2;  // 跳过 '%' 后面的两个字符
            break;
        case '&':
            // 遇到 '&' 表示一个键值对的结束，另一个键值对的开始
            value = body_.substr(j, i - j);  // 提取值
            j = i + 1;  // 更新下一个键值对的开始位置
            post_[key] = value;  // 存储键值对
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());  // 打印日志
            break;
        default:
            break;
        }
    }
    assert(j <= i);  // 确保起始位置不超过当前字符位置
    // 处理最后一个键值对
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

// 将字符转换为对应的十六进制值
int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;  // 如果是数字字符，则返回其对应值
}

// 检查用户登录
bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
    // 检查用户名或密码是否为空
    if(name == "" || pwd == "") { 
        return false;  // 如果用户名或密码为空，则返回false
    }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());

    MYSQL* sql;
    // 获取数据库连接
    SqlConnRAII(&sql, SqlConnPool::Instance());  
    assert(sql);  // 确保获取到了数据库连接

    bool flag = false;  // 用于存储验证结果
    char order[256] = { 0 };  // 用于存储SQL查询语句
    MYSQL_RES *res = nullptr;  // 存储查询结果

    // 如果是注册操作，则默认为成功状态
    if(!isLogin) { 
        flag = true; 
    }
    
    // 构造SQL查询语句，查找用户信息
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    // 执行SQL查询
    if(mysql_query(sql, order)) { 
        mysql_free_result(res);  // 释放查询结果
        return false;  // 查询失败，返回false
    }
    
    // 获取查询结果
    res = mysql_store_result(sql);

    // 遍历查询结果
    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);  // 获取查询到的密码

        // 处理登录操作
        if(isLogin) {
            if(pwd == password) { 
                flag = true;  // 密码匹配成功
            } else {
                flag = false;  // 密码不匹配
                LOG_DEBUG("pwd error!");
            }
        } 
        // 处理注册操作
        else { 
            flag = false;  // 默认用户已存在
            LOG_DEBUG("user used!");
        }
    }
    
    // 释放查询结果
    mysql_free_result(res);

    // 如果是注册操作且用户未存在，则插入新用户
    if(!isLogin && flag == true) {
        LOG_DEBUG("register!");
        // 构造SQL插入语句
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);

        // 执行SQL插入
        if(mysql_query(sql, order)) { 
            LOG_DEBUG("Insert error!");
            flag = false;  // 插入失败
        }
        flag = true;  // 插入成功
    }
    
    // 释放数据库连接
    SqlConnPool::Instance()->FreeConn(sql);  
    LOG_DEBUG("UserVerify success!!");
    return flag;  // 返回验证结果
}

string HttpRequest::path() const {
    return path_;
}

string& HttpRequest::path() {
    return path_;
}

string HttpRequest::method() const {
    return method_;
}

string HttpRequest::version() const {
    return version_;
}

// 获取 POST 数据中的值（通过 std::string 类型的 key）
string HttpRequest::GetPost(const string& key) const {
    assert(key != "");  // 确保 key 不为空
    if(post_.count(key) == 1) {
        return post_.find(key)->second;  // 获取 POST 数据中的值
    }
    return "";  // 如果 key 不存在，返回空字符串
}

// 获取 POST 数据中的值（通过 const char* 类型的 key）
string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);  // 确保 key 不为空指针
    if(post_.count(key) == 1) {
        return post_.find(key)->second;  // 获取 POST 数据中的值
    }
    return "";  // 如果 key 不存在，返回空字符串
}
