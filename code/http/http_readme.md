<a name="vbkxN"></a>
# 有限状态机
http报文的解析主要是使用**有限状态机**来实现，解析过程中需要使用到**正则表达式**来得到各个部分的数据。下面是大致的流程图：<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723270350963-8ac076bc-09a6-4a3b-aad5-94c0f0b9119f.png#averageHue=%23a18f7f&clientId=uededeb4d-7da6-4&from=paste&id=u358c484d&originHeight=739&originWidth=861&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u7bffb670-35d1-400d-958e-0f1417f0712&title=)
<a name="kL6cW"></a>
## 简单介绍正则表达式

- `used?`

?代表前面的一个字符可以出现0次或1次，说简单点就是d可有可无。

- `ab*c`

*代表前面的一个字符可以出现0次或多次

- `ab+c`

+会匹配出现1次以上的字符

- `ab{6}c`

指定出现的次数，比如这里就是abbbbbbc，同理还有ab{2,6}c：限定出现次数为2~6次；ab{2,}c为出现2次		以上

- `(ab)+`

这里的括号会将ab一起括起来当成整体来匹配

- `a (cat|dog)`

或运算：括号必不可少，可以匹配a cat和a dog

- `[abc]+`

或运算：要求匹配的字符只能是[]里面的内容；[a-z]匹配小写字符；[a-zA-Z]：匹配所有字母；a-zA-Z0-9：匹配所有字母和数字

- `.*`

. 表示 匹配除换行符 \n 之外的任何单字符，*表示零次或多次。所以.*在一起就表示任意字符出现零次或多次。

- `^`限定开头；取反：[^a]表示“匹配除了a的任意字符”，只要不是在[]里面都是限定开头
- `$`匹配行尾。如^a只会匹配行首的a，a$只会匹配行尾的a
- `?`

表示将贪婪匹配->懒惰匹配。就是匹配尽可能少的字符。就意味着匹配任意数量的重复，但是在能使整个匹配成功的前提下使用最少的重复。
<a name="EeYVj"></a>
## http请求报文
**HTTP请求报文包括请求行、请求头部、空行和请求数据四个部分。**

 ![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723269925383-ded73b43-3d3f-46dc-8c5e-aa8773f6df4b.png#averageHue=%23dadada&clientId=uededeb4d-7da6-4&from=paste&id=uc03ea233&originHeight=165&originWidth=466&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u9c747a08-4526-4f91-b9ba-b7c2782861c&title=)<br />以下是百度的请求包：
> GET / HTTP/1.1
> Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,/;q=0.8,application/signed-exchange;v=b3;q=0.9
> Accept-Encoding: gzip, deflate, br
> Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
> Connection: keep-alive
> Host: www.baidu.com
> Sec-Fetch-Dest: document
> Sec-Fetch-Mode: navigate
> Sec-Fetch-Site: none
> Sec-Fetch-User: ?1
> Upgrade-Insecure-Requests: 1
> User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.4951.41 Safari/537.36 Edg/101.0.1210.32
> sec-ch-ua: " Not A;Brand";v=“99”, “Chromium”;v=“101”, “Microsoft Edge”;v=“101”
> sec-ch-ua-mobile: ?0
> sec-ch-ua-platform: “Windows”

> **上面只包括请求行、请求头和空行，请求数据为空。请求方法是GET，协议版本是HTTP/1.1；请求头是键值对的形式。**

<a name="ghKIl"></a>
## 有限状态机
整个解析过程由`parse()`函数完成；函数根据http请求报文分别调用三个不同函数
```cpp
ParseRequestLine_();//解析请求行
ParseHeader_();//解析请求头
ParseBody_();//解析请求体
```
![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723270126376-8322112a-f247-4a85-b5be-7921848a28dd.png#averageHue=%231e1e1e&clientId=uededeb4d-7da6-4&from=paste&height=431&id=uf809f42b&originHeight=646&originWidth=744&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=36884&status=done&style=none&taskId=u29bcbd1b-5f74-4261-9495-12637a98ea7&title=&width=496)<br />三个函数对请求行、请求头和数据体进行解析。当然解析请求体的函数还会调用`ParsePost_()`，因为Post请求会携带请求体。<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723270208594-135b739c-a239-4434-a445-f2cdc477b603.png#averageHue=%23f9f8f8&clientId=uededeb4d-7da6-4&from=paste&id=u8d3e5e75&originHeight=354&originWidth=841&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u08dc4d82-3ec1-42d2-8fc4-15059dc9773&title=)
<a name="u7hK2"></a>
# HTTP连接
<a name="qAlC7"></a>
## http响应报文
响应报文组成：
```cpp
HTTP/1.1 200 OK
Date: Fri, 22 May 2009 06:07:21 GMT
Content-Type: text/html; charset=UTF-8
\r\n
<html>
      <head></head>
      <body>
            <!--body goes here-->
      </body>
</html>
```

- 状态行，由HTTP协议版本号， 状态码， 状态消息 三部分组成。 第一行为状态行，（HTTP/1.1）表明HTTP版本为1.1版本，状态码为200，状态消息为OK。
- 消息报头，用来说明客户端要使用的一些附加信息。 第二行和第三行为消息报头，Date:生成响应的日期和时间；Content-Type:指定了MIME类型的HTML(text/html),编码类型是UTF-8。
- 空行，消息报头后面的空行是必须的。**它的作用是通过一个空行，告诉服务器头部到此为止**。
- 响应正文，服务器返回给客户端的文本信息。空行后面的html部分为响应正文。
<a name="xQvTP"></a>
## 实现响应报文步骤
在`HttpResponse.cpp`中，主要是靠`MakeResponse(Buffer& buff)`这个函数实现的，分为三个步骤：1. 添加状态行；2. 添加头部；3. **添加响应报文**，其中，第三步涉及到内存映射。<br />我们在`HttpRequest.cpp`中生成了请求报文，请求报文里面包括了我们要请求的页面（path），响应报文的过程就是根据请求报文写出响应头，将path上的文件放在响应体，然后发送给客户端（浏览器）。
<a name="Dr0Aw"></a>
### 生成状态码
```cpp
if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }
```
关于stat的使用，参考这篇博文：[stat函数-CSDN博客](https://blog.csdn.net/Dustinthewine/article/details/126673326)
<a name="gYmXP"></a>
### 添加状态行和头部
这里主要就是一些响应报文基本的内容，用`buff.Append()`添加进去即可。<br />状态行只需要添加文件对应状态的状态码即可<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723294453394-56f46875-7fbe-4470-bb5f-a8db774c530e.png#averageHue=%231f1e1e&clientId=u38251e1b-1caf-4&from=paste&height=296&id=u014f10a5&originHeight=489&originWidth=1153&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=73945&status=done&style=none&taskId=u4dbdfd2f-ad28-4aa0-9074-433d9537a9a&title=&width=698.7878383990294)<br />http头部也只需添加对应的文件类型和长度等信息<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723294619886-be8cfde1-3919-405c-b289-87123a74f73c.png#averageHue=%2321201f&clientId=u38251e1b-1caf-4&from=paste&height=304&id=u08f9457c&originHeight=502&originWidth=1027&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=62473&status=done&style=none&taskId=udebfd815-fed6-425e-b7dd-a6eba590cd7&title=&width=622.4242064490921)
<a name="qsY9s"></a>
### 添加响应正文
```cpp
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
```

1. 首先需要打开这个文件
2. 用`mmap`这个函数映射到内存，可以提高文件的访问速度
3. 映射过后，就可以将该文件描述符关闭了
4. 将该内存映射的指针加入`buff`，至此，报文的所有内容就都在buff中了，后续读取到浏览器上，浏览器解析渲染就得到我们所看到的内容了。
<a name="EQUaO"></a>
## http连接
<a name="Ihdci"></a>
### 整体分析
一个HTTP连接要实现的功能就是：

1. 读取请求
2. 解析请求
3. 生成响应
4. 发送响应

然后浏览器就可以进行解析渲染了。<br />先看一下大致的流程图：<br />![](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723294809290-53e71fa4-7917-4631-af9a-d7deed485b59.png#averageHue=%23bbb1a7&clientId=u38251e1b-1caf-4&from=paste&id=u92e83b31&originHeight=725&originWidth=708&originalType=url&ratio=1.5&rotation=0&showTitle=false&status=done&style=none&taskId=u200d9d6d-08a4-4551-b6ad-dc5067bdb0d&title=)<br />在一个HttpConn中，有两个缓冲区，一个用来读，一个用来写，还分别有个**request_** （将请求报文放在读缓冲区） 和 **response_**(将响应报文放在写缓冲区)。<br />注意理清这里的读和写的含义，因为请求报文是浏览器发来的，所以需要用读缓冲区去读取，而响应报文是这个连接生成准备发给浏览器的，所以对应写缓冲区<br />解析请求报文和生成响应报文都是在`HttpConn::process()`函数内完成的。并且是在解析请求报文后随即生成了响应报文。之后这个生成的响应报文便放在缓冲区等待`writev()`函数将其发送给fd。
<a name="lNFB8"></a>
### HttpConn的读写
在这段代码里面，还有两个重要的函数，分别是`read`和`write`<br />这里的`read`需要将请求报文的内容读取到读缓冲区里面来，这样request_才能够解析报文。<br />然后是`write`，这里主要是使用了`writev`连续写来将响应报文写到fd（浏览器）中。我们前面提到的生成响应报文，他只是放在了buff中，并没有发送给浏览器，这里用write就实现传输啦。<br />注意由于生成响应报文的响应正文里面只是生成了`Content-length`，并没有将文件也放进缓冲区，因为如果文件太大，缓冲区可能会装不下。所以我们在传输的时候，采用了分块写的方式，一块传输buff里面的内容，另一块传输内存映射的文件指针。<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723295349885-a8ef4f72-c950-4947-b4af-04acbb05e4e3.png#averageHue=%23201f1f&clientId=u38251e1b-1caf-4&from=paste&height=105&id=uf959f3f4&originHeight=174&originWidth=1296&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=30282&status=done&style=none&taskId=u6b53e33f-02e6-4faf-b71f-236f7628782&title=&width=785.454500056498)<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723295269989-94df3783-7048-42eb-b011-23c4c36de3f8.png#averageHue=%23201f1f&clientId=u38251e1b-1caf-4&from=paste&height=421&id=u25452319&originHeight=694&originWidth=1165&originalType=binary&ratio=1.5&rotation=0&showTitle=false&size=113181&status=done&style=none&taskId=u5760a532-8cb1-43cc-a5aa-c04804b642a&title=&width=706.0605652514045)
<a name="zC3zP"></a>
### 逻辑代码
```cpp
//只为了说明逻辑，代码有删减
bool HttpConn::process() {
    request_.Init();//初始化解析类
    if(readBuff_.ReadableBytes() <= 0) {//从缓冲区中读数据
        return false;
    }
    else if(request_.parse(readBuff_)) {//解析数据,根据解析结果进行响应类的初始化
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        response_.Init(srcDir, request_.path(), false, 400);
    }
    response_.MakeResponse(writeBuff_);//生成响应报文放入writeBuff_中
    /* 响应头  iov记录了需要把数据从缓冲区发送出去的相关信息
    iov_base为缓冲区首地址，iov_len为缓冲区长度 */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if(response_.FileLen() > 0  && response_.File()) { //
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    return true;
}
```