## 一、前言
&emsp;&emsp;软件使用纯C语言编写，完全开源，没有调用库文件。程序中使用了自定义的链表、线程池、日志、字符串处理等模块，是一个适合初学者学习练手的项目
&emsp;&emsp;开源地址：https://github.com/jiangbfy/chttp-server

## 二、HTTP功能
- 1、查询用户信息
```
GET /api/user/all
```
```
"code": 200,
"message": "usccess",
"data": [
    {
        "id": "1",
         "user": "root",
        "passwd": "root",
        "time": "2024-6-3 17:13:54"
     },
     {
         "id": "2",
         "user": "admin",
         "passwd": "admin",
        "time": "2024-6-4 13:43:34"
    },
    {
         "id": "3",
         "user": "imx6ull",
         "passwd": "imx6ull",
         "time": "2024-6-5 23:49:30"
     },
    {
        "id": "4",
         "user": "rk3588",
         "passwd": "rk3588",
         "time": "2024-7-5 12:22:46"
     },
]
```

- 2、下载图片
```
GET /api/file/download/static/lurennvzhu.jpg
```
<div style="text-align: center;"><img src="https://jiangbfy.com/static/image/2025-2/1738723949311.png01.png"/></div>

- 3、上传图片
```
POST /api/file/upload
Content-Type: multipart/form-data;
```
```
"code": 200,
"url": /static/lurennvzhu.jpg
```

## 三、日志功能
&emsp;&emsp;日志分为4个等级：debug，info，warn和error。日志文件以XX_XX_XX的年月日形式保存，保存到工作路径的logs目录下

## 四、编译运行
- 数据库初始化
```
CREATE TABLE "user" (
  "id" INTEGER PRIMARY KEY AUTOINCREMENT,
  "user" TEXT NOT NULL,
  "passwd" TEXT NOT NULL,
  "time" TEXT NOT NULL
);
INSERT INTO "user" VALUES (1, 'root', 'root', '2024-6-3 17:13:54');
INSERT INTO "user" VALUES (2, 'admin', 'admin', '2024-6-4 13:43:34');
INSERT INTO "user" VALUES (3, 'imx6ull', 'imx6ull', '2024-6-5 23:49:30');
INSERT INTO "user" VALUES (4, 'rk3588', 'rk3588', '2024-7-5 12:22:46');
```
- 编译运行
```
mkdir logs
make
./server
```
