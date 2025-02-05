#ifndef STORE_H
#define STORE_H

#include "webserver.h"

// 路径长度
#define PATH_LEN 512

// 全局变量
typedef struct SysPara {
    // Web服务器实例
    WebServer *server;
    // 程序路径
    char workPath[PATH_LEN];
}SysPara;

// 设置程序路径
void SetWorkPath(char *path);
extern SysPara syspara;

// 全局变量赋值
#define ParaCommit(para) syspara.para = para
// 读取全局变量
#define ParaCheck(para)  syspara.para

#endif