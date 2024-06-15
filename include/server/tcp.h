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
#include "def.h"
#include "http.h"

// TCP发送和接收缓冲数组大小
#define BUF_LEN 2048
// iovec结构体数量，用于发送和接收文件
#define IOVEC_LEN 8

typedef struct Tcp
{
    // TCP接收数据长度
    int m_rxLen;
    // TCP发送数据长度
    int m_txLen;
    // TCP套接字
    int m_sockfd;
    // epoll套接字
    int m_epollfd;
    // TCP连接保持时间
    int m_expire;
    // TCP客户端ip地址和端口
    struct sockaddr_in m_address;
    // 接收缓冲数组
    char m_rxBuf[BUF_LEN];
    // 发送缓冲数组
    char m_txBuf[BUF_LEN];
    // 接收文件
    struct iovec m_rxiov[IOVEC_LEN];
    // 发送文件
    struct iovec m_txiov[IOVEC_LEN];

    // TCP结构体配置 (epoll套接字, TCP 套接字, 客户端ip地址和端口, TCP连接保持时间)
    void (*Config)(struct Tcp *this, int epollfd, int sockfd, struct sockaddr_in *addr, int expire);
    // 接收数据，线程池中处理
    void (*Recv)(struct Tcp *this);
    // 处理数据
    void (*Process)(struct Tcp *this);
    // 发送数据，线程池中处理
    void (*Send)(struct Tcp *this);
} Tcp;

Tcp* TcpInit(void);
void TcpRelease(Tcp* this);

#endif