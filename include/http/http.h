#ifndef HTTP_H
#define HTTP_H

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "def.h"
#include "map.h"

// TCP发送和接收缓冲数组大小
#define BUF_LEN 2048
// TCP一次性接收长度
#define RECV_LEN 2000
// 应答head长度
#define RESP_HEAD_LEN 2048

// GET,POST,OPTION
typedef enum METHOD
{
    METHOD_NULL = 0,
    METHOD_GET,
    METHOD_POST,
    METHOD_OPTIONS,
} METHOD;

typedef enum STATE
{
    STATE_REQ = 0,
    STATE_HEAD,
    STATE_BODY,
    STATE_ERR
} STATE;

typedef struct Http
{
    // TCP套接字
    int m_sockfd;
    // http方法
    METHOD m_method;
    // 解析http状态
    STATE m_state;
    // http url param
    char *m_url;
    // http body
    char *m_body;
    // http body length
    int m_bodyLen;
    // response head
    char *m_respHead;
    // response body
    char *m_respBody;
    // response head length
    int m_respHeadLen;
    // response body length
    int m_respBodyLen;
    // 文件类型
    char *m_fileType;
    // ip地址和端口
    struct sockaddr_in m_address;
    // HTTP头
    KVList *m_header;
    KVList *m_param;
    // 匹配函数
    void (*function)(struct Http *http);

    // 接收http数据
    void (*Recv)(struct Http *this);
    // 配置
    void (*Config)(struct Http *this, int sockfd, struct sockaddr_in *addr);
} Http;

typedef struct Service
{
    // 第二段匹配url
    char *m_second;
    // http 方法
    METHOD m_method;
    // 匹配函数
    void (*function)(Http *http);
} Service;

typedef struct ServiceClass
{
    // 第一段匹配url
    char *m_first;
    // 匹配结构体
    const Service *m_service;
} ServiceClass;

Http* HttpInit(void);
void  HttpRelease(Http *this);

#endif