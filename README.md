# [:frog:基于云服务器的TCP网络聊天室](https://github.com/Realself-Ma/TcpChatRoom)
> 客户端Qt5.6开发，后台服务器基于muduo网络库实现，Mysql数据库管理用户信息
>

| Part Ⅰ                                                       | Part Ⅱ                                                       | Part Ⅲ                                                       | Part Ⅳ                                                       | Part Ⅴ                                                       | Part Ⅵ                                                       | Part Ⅶ                                                       |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| [项目目的](https://github.com/Realself-Ma/TcpChatRoom/blob/master/%E9%A1%B9%E7%9B%AE%E7%9B%AE%E7%9A%84.md) | [整体结构](https://github.com/Realself-Ma/TcpChatRoom/blob/master/%E6%95%B4%E4%BD%93%E7%BB%93%E6%9E%84.md) | [版本历史](https://github.com/Realself-Ma/TcpChatRoom/blob/master/%E7%89%88%E6%9C%AC%E5%8E%86%E5%8F%B2.md) | [其它说明](https://github.com/Realself-Ma/TcpChatRoom/blob/master/%E5%85%B6%E5%AE%83%E8%AF%B4%E6%98%8E.md) | [遇到的困难](https://github.com/Realself-Ma/TcpChatRoom/blob/master/%E9%81%87%E5%88%B0%E7%9A%84%E5%9B%B0%E9%9A%BE.md) | [测试及优化](https://github.com/Realself-Ma/TcpChatRoom/blob/master/%E6%B5%8B%E8%AF%95%E5%8F%8A%E4%BC%98%E5%8C%96.md) | [基础知识](https://github.com/Realself-Ma/TcpChatRoom/blob/master/%E5%9F%BA%E7%A1%80%E7%9F%A5%E8%AF%86.md) |

### 开发环境

- 服务器操作系统：Ubuntu 18.04
- 编译器：gcc 7.4.0

### 其它

- 加入了自动重启服务端程序的Shell脚本 -restart.sh,可实现自动重启程序，配合screen 命令，可以实现服务器的常态运行
- [视频介绍](https://www.bilibili.com/video/av74080010)