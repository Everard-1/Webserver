<a name="qxKkJ"></a>
# 压力测试
更多内容可以参考：[WebServer 跑通/运行/测试（详解版）_webserver测试-CSDN博客](https://blog.csdn.net/csdner250/article/details/135503751?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522172343072816800227442681%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=172343072816800227442681&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-1-135503751-null-null.142^v100^pc_search_result_base5&utm_term=WebServer%E5%8E%8B%E5%8A%9B%E6%B5%8B%E8%AF%95&spm=1018.2226.3001.4187)<br />我们需要先安装依赖
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
可以看到我们很轻松就实现了20000并发<br />![image.png](https://cdn.nlark.com/yuque/0/2024/png/27393008/1723440288905-4b47393d-6ce8-4186-91c0-75d234bf8ac9.png#averageHue=%23212020&clientId=ue84fba2c-5908-4&from=paste&height=165&id=u007b6c14&originHeight=273&originWidth=1047&originalType=binary&ratio=1.6500000953674316&rotation=0&showTitle=false&size=30366&status=done&style=none&taskId=u8158acaf-1870-4b06-bf89-d0b8fb6d009&title=&width=634.5454178697171)<br />内存：8G，CPU：R9-5900H ,Ubuntu:18.04
> 注意：如果内存只有4G，QPS差不多在10000左右，因为虚拟机的性能会限制进程数量