#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "def.h"
#include "cJSON.h"

#define RES_BODY 512

// GET,POST,OPTION
typedef enum METHOD
{
    METHOD_GET = 0,
    METHOD_POST,
    METHOD_OPTION,
    METHOD_NULL
} METHOD;

// 解析http结果，正常，错误，完成
typedef enum LINE_STATE
{
    LINE_OK = 0,
    LINE_ERROR,
    LINE_FIN
} LINE_STATE;

typedef struct Http
{
    // http方法
    METHOD m_method;
    // 解析http结果
    LINE_STATE m_line_state;
    // http url
    char *m_url;
    // http body
    char *m_body;
    // ip地址和端口
    struct sockaddr_in *m_address;

    // 解析http请求 (http请求报文，http请求长度)
    void (*ProcessRequest)(struct Http *this, char *request, int reqLen);
    // 处理应答 (处理请求结果)
    void (*ProcessResponse)(struct Http *this, char *response);
} Http;

Http* HttpInit(void);
void  HttpRelease(Http *this);
// 解析url，将参数放到json对象中
int ParseUrl(char *url, cJSON *object);

#endif