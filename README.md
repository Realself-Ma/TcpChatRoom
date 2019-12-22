# TCP-ChatRoom
基于云服务器的TCP网络聊天室【客户端Qt5.6开发、后台服务器基于muduo网络库程实现、Mysql数据库管理用户信息】
 - **客户端**
 
 win10下的客户端
 
![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022213925511.png)

Ubuntu下客户端

![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022214033201.png)

聊天室

![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022214506319.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQzMzY1ODI1,size_16,color_FFFFFF,t_70)

接收到好友请求

![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022214824916.png)

是否同意？拒绝后，发送请求的一方会受到拒绝信息。

![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022214850469.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQzMzY1ODI1,size_16,color_FFFFFF,t_70)

更换头像，实时更新，将头像信息回传到服务器。

![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022215305872.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQzMzY1ODI1,size_16,color_FFFFFF,t_70)

实时更新好友头像，和状态信息

![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022215650713.png)

 - **服务器端**
 
 服务器后台记录在这里插入图片描述
 
 ![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022214118311.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQzMzY1ODI1,size_16,color_FFFFFF,t_70)

服务器数据库

![在这里插入图片描述](https://img-blog.csdnimg.cn/20191022214157655.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQzMzY1ODI1,size_16,color_FFFFFF,t_70)

 - **注意**
 
日志库spdlog需要把其文件夹中的include/spdlog 文件夹放到Linux系统/usr/include 目录下

数据库相关函数需要mysql.h头文件，需要Linux系统安装mysql软件

sudo apt-get install mysql-server

sudo apt-get install libmysqlclient-dev[开发包，不安装会提示找不到mysql.h头文件]

我用的数据库名称叫做ChatRoom，若你的mysql中没有这一数据库，会提示连接数据库失败

保存用户信息的表的结构，在上面给出的图片可以看到

服务器IP为云服务器IP，这样可以保证公网下的用户都能连接上，测试时要修改为你的本地IP，客户端，服务器端都要改


- **2019/12/19日更新-基于muduo网络库重构服务端代码**
![多用户同时在线](https://img-blog.csdnimg.cn/20191222111142621.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQzMzY1ODI1,size_16,color_FFFFFF,t_70)

 	1.并发连接数增加，业务逻辑与服务逻辑区分开，客户端的默认端口是2019
 
 	2.加入了压力测试代码-ChatRoom_loadtest.cc，可以进行压力测试
 	![在这里插入图片描述](https://img-blog.csdnimg.cn/20191222112035105.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQzMzY1ODI1,size_16,color_FFFFFF,t_70)
	
	3.加入了自动重启服务端程序的Shell脚本-restart.sh,可实现自动重启程序，配				  合·screen 命令，可以实现服务器的常态运行
 
 
 参考了muduo网络库 https://github.com/chenshuo/muduo
