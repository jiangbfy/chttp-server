#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "http.h"

typedef struct Service
{
    // 第二段匹配url
    char *m_SecondUrl;
    // 匹配函数
    int (*ParseReq)(char *argOut, Http *this);
} Service;

typedef struct Controller
{
    // 第一段匹配url
    char *m_fristUrl;
    // 匹配结构体
    const Service *m_service;
} Controller;

// 根据第一段url先匹配到对应的结构体，根据第二段url匹配到对应的函数 (处理请求结果，http结构体)
int ControllerParse(char *argOut, Http *this);

#endif