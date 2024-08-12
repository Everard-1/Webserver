<a name="PQwUF"></a>
# main函数配置启动
main.cpp
```cpp
#include <unistd.h>
#include "server/webserver.h"

int main() {
    /* 守护进程 后台运行 */ 
    //daemon(1,0);

    WebServer server(
        1316, 3, 60000, false,             /* 端口 ET模式 timeoutMs 优雅退出  */
        3306, "ct", "P@ssw0rd2024", "yourdb", /* Mysql配置 */
        12, 6, true, 1, 1024);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.Start();
} 
  
```
这里我们需要配置MYSQL中的用户名，密码和你创建的数据库名（要和你在Linux中创建的对应，不然无法再浏览器注册和登录）<br />Makefile（build中的makefile）
```cpp
CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = server2
OBJS = ../code/log/*.cpp ../code/pool/*.cpp ../code/timer/*.cpp \
       ../code/http/*.cpp ../code/server/*.cpp \
       ../code/buffer/*.cpp ../code/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)
```
<a name="qxKkJ"></a>