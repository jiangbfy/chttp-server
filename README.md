# chttp-server
## 简介
&emsp;&emsp;Linux下以C语言实现的轻量级http服务器，提供一种嵌入式环境下前后端分离的后端方案，直接arm-linux-gnueabihf-gcc交叉编译就可以移植到开发板上运行。另一方面，可以帮助新手学习网络编程。
+ 使用线程池并行处理不同http请求
+ 使用epoll(IO多路复用)，socket设置为非阻塞
+ 使用数据库连接池，因为sqlite3直接对数据库文件进行操作，不支持多个连接，所以数量为1
+ 使用二叉树管理TCP连接，二叉树key为sockfd
+ 日志模块记录程序运行状态
+ 实现GET和POST请求，可解析url参数和json参数
+ Controller根据第一段url匹配不同服务类，再根据第二段url匹配不同方法(例：/infor/userinfor，根据/infor找到user_service，根据/userinfor找到UserInfo方法，查询用户信息)

## 运行
+ 服务器环境
	+ ubuntu18.04(可交叉编译，arm开发板中运行)
+ 测试环境
	+ postman
+ 数据库初始化
```
CREATE TABLE userinfo (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  user TEXT NOT NULL,
  passwd TEXT NOT NULL,
  time TEXT NOT NULL
);
```
+ 配置
	+ 在main.c中配置服务器端口号，数据库名和线程池数量
+ 编译
	+ 运行`make`
	+ 新建目录`logs`
	+ 运行`./server`