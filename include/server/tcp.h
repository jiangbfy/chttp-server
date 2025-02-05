#ifndef TCP_H
#define TCP_H

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include "http.h"
#include "def.h"

typedef struct Tcp
{
    // TCP套接字
    int m_sockfd;
    // epoll套接字
    int m_epollfd;
    // TCP客户端ip地址和端口
    struct sockaddr_in m_address;
    // HTTP实例
    Http *m_http;

    // TCP结构体配置 (epoll套接字, TCP 套接字, 客户端ip地址和端口, TCP连接保持时间)
    void (*Config)(struct Tcp *this, int epollfd, int sockfd, struct sockaddr_in *addr);
    // 接收数据，线程池中处理
    void (*Recv)(struct Tcp *this);
    // 发送数据，线程池中处理
    void (*Send)(struct Tcp *this);
} Tcp;

Tcp* TcpInit(void);
void TcpRelease(Tcp* this);
void modfd(int epollfd, int fd, int ev, int one_shot, int trigMode);

#endif