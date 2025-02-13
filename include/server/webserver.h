#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include "def.h"
#include "tcp.h"
#include "threadpool.h"
#include "sqlpool.h"

// epoll支持的最大并发量
#define EPOLL_SIZE 1000
// TCP支持的最大数量
#define TCP_SIZE 1000
// alarm定时周期
#define TIMEOUT    1
// TCP超时时间
#define EXPIRE     10

typedef struct ListNode
{
    // TCP套接字
    int id;
    // 节点深度
    int timeout;
    // 用户数据，指向tcp控制结构体
    void *data;
    // 下一个节点
    struct ListNode *next;
    // 上一个节点
    struct ListNode *prev;
} ListNode;

typedef struct List
{
    // List头节点
    ListNode *m_head;
    // List尾节点
    ListNode *m_end;
    // List节点数量
    int m_size;
    // 增加节点
    void (*Push)(struct List *this, int id, void *data);
    // 删除节点
    void (*Pop)(struct List *this, int id);
    // 查找节点
    ListNode* (*Find)(struct List *this, int id);
    // 检查节点超时
    void (*Check)(struct List *this, int time);
} List;

typedef struct WebServer
{
    // 服务器端口
    int m_port;
    // 服务器套接字
    int m_listenfd;
    // epoll套接字
    int m_epollfd;
    // 管道套接字
    int m_pipefd[2];
    // alarm定时周期是否到达
    bool isTimeout;
    // m_stack的栈指针
    int m_stackSize;
    // 线程池数量
    int m_taskNum;
    // 存储超时tcp连接的栈，统一删除
    int m_stack[EPOLL_SIZE];
    // epoll事件结构体
    struct epoll_event m_events[EPOLL_SIZE];
    // 互斥锁
    pthread_mutex_t m_mutex;

    // 数据库名
    char *m_dbName;
    // 链表，存储TCP连接
    List *m_list;
    // 线程池
    ThreadPool *m_thpool;
    // 数据库连接池
    SqlPool *m_sqlpool;

    // 服务器配置(服务器端口, 数据库用户名, 数据库密码, 数据库名, 线程池数量)
    void (*Config)(struct WebServer *this, int port, char *dbName, int taskNum);
    // 服务器初始化
    void (*Init)(struct WebServer *this);
    // 服务器运行
    void (*Run)(struct WebServer *this);
} WebServer;

WebServer* WebServerInit(void);
void WebServerRelease(WebServer* this);

#endif