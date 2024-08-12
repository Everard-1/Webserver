<a name="Umcfo"></a>
# MySQL
先去csdn搜索如何在Linux中安装MySQL，这里推荐几个<br />[c++ 经典服务器开源项目 Tinywebserver的使用与配置(百度智能云服务器安装ubuntu18.04可用公网ip访问)-CSDN博客](https://blog.csdn.net/yingLGG/article/details/121400284?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522172344520516800180651394%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=172344520516800180651394&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~baidu_landing_v2~default-5-121400284-null-null.142^v100^pc_search_result_base5&utm_term=TinyWebserver%E7%8E%AF%E5%A2%83&spm=1018.2226.3001.4187)<br />[（一）TinyWebServer的环境配置与运行-CSDN博客](https://blog.csdn.net/weixin_46653651/article/details/133420059?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522172344520516800180651394%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=172344520516800180651394&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-1-133420059-null-null.142^v100^pc_search_result_base5&utm_term=TinyWebserver%E7%8E%AF%E5%A2%83&spm=1018.2226.3001.4187)<br />安装好MySQL后你需要创建好一个自己的账号，密码，登录进你的数据库
```cpp
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, password) VALUES('name', 'password');	
```
下面是在你的数据库中建立一个yourdb数据库，在yourdb中又建一个user表，表中（VALUES括号中）就是你后面登录和注册使用的账户数据。
> 注意：上面的username和password选项不可以更改，因为我们代码中就是这两个选项，你更改了会找不到对应的数据，如果实在要修改，请去`**httprequest.cpp**`中自己更改。同时上面的步骤分部进行，且必须进入数据库后才能使用。前面配置数据库多看博客或问AI。

当然，你也可以下载一个Navicat连接到你的Linux的MySQL数据库，就像我这样<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723445840069-a19e855d-772b-475c-a591-3509fb154e7b.png#averageHue=%232f2e2e&clientId=u075ec968-b6b2-4&from=paste&height=809&id=ubae9a820&originHeight=1113&originWidth=1978&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=131101&status=done&style=none&taskId=u910af27b-dea0-47c6-a015-03350b1b69a&title=&width=1438.5454545454545)<br />但是这需要你自己去找博客如何下载安装Navicat并连接到Linux中
<a name="DqCxM"></a>
# 运行项目
数据库好了之后你就可以运行
```cpp
make
./bin/server2
```
因为我在修改了一下makefile文件，把输出改成了server2，而最开始的开源项目就是serve<br />然后你打开浏览器，输入你的IP地址+端口号，比如`[**192.168.92.133**](http://192.168.92.133:1316/)**:1316**`<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723446218921-dc59b2e3-19b3-464a-89d3-d317e8621b8c.png#averageHue=%231f1e1e&clientId=u075ec968-b6b2-4&from=paste&height=258&id=uba1258c2&originHeight=355&originWidth=1385&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=53014&status=done&style=none&taskId=u34c5f1d7-f0c1-4bce-8917-0bde4c620e2&title=&width=1007.2727272727273)
> 因为我跟着之前的项目一样设置的1316端口，当然你也可以更改后重新生成可执行文件运行

然后我们就可以看到画面了<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723446311672-e6150376-7cd5-44d3-9300-0a33bea42e76.png#averageHue=%23fefdfd&clientId=u075ec968-b6b2-4&from=paste&height=995&id=u67c1d68b&originHeight=1368&originWidth=2560&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=359996&status=done&style=none&taskId=u154c3331-9ae3-4301-ab21-98bd249f5d3&title=&width=1861.8181818181818)<br />然后你可以去测试你的登录注册功能，以及查看图片和视频功能<br />如果你注册登录有问题，说明你之前的数据库可能配置有问题<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723446377778-ee5bf252-de6d-4693-8e1a-6b1264fd6ed4.png#averageHue=%23313130&clientId=u075ec968-b6b2-4&from=paste&height=184&id=u2c3e6220&originHeight=253&originWidth=499&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=10382&status=done&style=none&taskId=uda09c602-01a7-421c-982b-10268da888c&title=&width=362.90909090909093)<br />下面是一些运行图片：<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723446626579-524f39ab-2eee-4949-9f7c-c083af5b7a3b.png#averageHue=%23efedec&clientId=u075ec968-b6b2-4&from=paste&height=916&id=uabb053d9&originHeight=1259&originWidth=2478&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=1562168&status=done&style=none&taskId=u75567e97-7a56-4d8c-aa57-d305d13b1b3&title=&width=1802.1818181818182)![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723446641457-2124e546-2832-40e3-bcfa-cbcab3181d96.png#averageHue=%23dedcda&clientId=u075ec968-b6b2-4&from=paste&height=916&id=u502a2827&originHeight=1259&originWidth=2478&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=1734848&status=done&style=none&taskId=u8a6bc1e2-c6bc-45cd-885d-26ff38c9fd9&title=&width=1802.1818181818182)<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723446644626-8fb9ef66-ff3c-4624-a7ff-a1c957880a16.png#averageHue=%23fefefe&clientId=u075ec968-b6b2-4&from=paste&height=916&id=u2a65de2f&originHeight=1259&originWidth=2478&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=83221&status=done&style=none&taskId=u088e9646-bf5d-49ee-a2ff-83e1f36635b&title=&width=1802.1818181818182)
<a name="W75NK"></a>
# 单元测试
这个就是简单的测试一下日志系统
```bash
cd test
make
./test
```
<a name="DjPEQ"></a>
# 压力测试
有问题可以参考这篇文章[WebServer 跑通/运行/测试（详解版）_webserver测试-CSDN博客](https://blog.csdn.net/csdner250/article/details/135503751?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522172344673016800222827225%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=172344673016800222827225&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-1-135503751-null-null.142^v100^pc_search_result_base5&utm_term=WebServer%E5%8E%8B%E5%8A%9B%E6%B5%8B%E8%AF%95&spm=1018.2226.3001.4187)<br />我们先安装相关的依赖：
```bash
sudo apt-get install exuberant-ctags
```
然后cd 到webbench的目录，直接make<br />然后我们可以进行测试：
```bash
webbench -c 500 -t 30 http://baidu.com/
```
![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723432401890-d6e94acb-d6c2-4669-889f-49dce25616be.png#averageHue=%23222120&clientId=u3d411463-ecdc-4&from=paste&id=ue7dc4bdd&originHeight=221&originWidth=1015&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=25969&status=done&style=none&taskId=ua5d244da-c487-4054-83f6-91fc99278a8&title=)<br />可以看到可以成功运行，接下来我们先把自己的服务器跑起来
```bash
./bin/server2
```
![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723440262764-04c64a41-0262-469d-a89b-d0596b38c178.png#averageHue=%231f1f1e&clientId=ue84fba2c-5908-4&from=paste&height=60&id=u0128f9d9&originHeight=99&originWidth=654&originalType=binary&ratio=1.6500000953674316&rotation=0&showTitle=false&size=5466&status=done&style=none&taskId=ue3aefa3f-29d5-4338-8263-fe7f138224f&title=&width=396.36361345443646)<br />再用这个来测试
```bash
webbench -c 20000 -t 10 http://192.168.92.133:1316/
```
可以看到我们很轻松就实现了20000并发，但这是我多次测试后的极限值，建议大家先选较少的测试
```bash
./webbench-1.5/webbench -c 100 -t 10 http://ip:port/
./webbench-1.5/webbench -c 1000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 5000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 10000 -t 10 http://ip:port/
```
![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723440288905-4b47393d-6ce8-4186-91c0-75d234bf8ac9.png#averageHue=%23212020&clientId=ue84fba2c-5908-4&from=paste&height=165&id=u007b6c14&originHeight=273&originWidth=1047&originalType=binary&ratio=1.6500000953674316&rotation=0&showTitle=false&size=30366&status=done&style=none&taskId=u8158acaf-1870-4b06-bf89-d0b8fb6d009&title=&width=634.5454178697171)<br />内存：8G，CPU：R9-5900H ,Ubuntu:18.04
> 注意：如果内存只有4G，QPS差不多在10000左右，因为虚拟机的性能会限制进程数量

特别注意：当你要求并发的进行数大于系统所能支持后，他会报错说资源不够，然后后面你就发现只能并发很少数量的进程了，因为压力测试成功后他会自动销毁子进程，但是失败后不会，此时系统中有大量的无用子进程<br />我们需要先用`**ps ajx**`查看他们的组id<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723447145044-018a8399-28d3-4bfa-a999-2fe7b8ece00d.png#averageHue=%232d0a24&clientId=u075ec968-b6b2-4&from=paste&height=697&id=ue86e2f9b&originHeight=959&originWidth=1527&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=299319&status=done&style=none&taskId=u94e35d05-efda-4b79-b72c-51ff7b2a219&title=&width=1110.5454545454545)<br />然后杀死这一组的进程，用`**kill -TERM -86787(组id)**`<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723447235644-a03d511e-77f2-452c-a83b-ba7fc58045f0.png#averageHue=%232d0923&clientId=u075ec968-b6b2-4&from=paste&height=689&id=u6b3e3ef2&originHeight=948&originWidth=1521&originalType=binary&ratio=1.375&rotation=0&showTitle=false&size=260485&status=done&style=none&taskId=ua1e33745-d348-4c78-8749-52b0ec7e5e2&title=&width=1106.1818181818182)<br />可以看到我们的进程又少了，此时再进行压力测试就可以得到比较高的并发数

好的，基本上配置有关的内容就这些，希望大家不懂的多查资料，我们都是这样过来的，加油！
