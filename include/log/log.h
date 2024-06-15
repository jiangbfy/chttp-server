#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "def.h"

#define LOG_LEN 1024

typedef struct MsgList
{
    char *msg;
    struct MsgList *next;
} MsgList;

typedef struct Log
{
    // 用链表存储log日志，m_frist是链表头，m_end是链表尾
    MsgList *m_frist;
    MsgList *m_end;
    // 日志数量
    int m_size;
    // 打开的日志文件
    FILE *m_fp;
    // 线程退出标志
    int m_shutdown;
    // 关闭日志
    int m_closeLog;
    // 当前日期
    int m_today;
    // 互斥锁
    pthread_mutex_t m_mutex;

    // 添加日志
    void (*AddMsg)(struct Log *this, char *msg, int len);
    // 添加日志
    void (*PrintLog)(struct Log *this, int level, const char *format, ...);
} Log;

extern Log *LogInstance;

void LogInit(void);
void LogRelease(void);

#define LOG_DEBUG(format, ...) if(0 == LogInstance->m_closeLog) {LogInstance->PrintLog(LogInstance, 0, format, ##__VA_ARGS__);}
#define LOG_INFO(format, ...) if(0 == LogInstance->m_closeLog) {LogInstance->PrintLog(LogInstance, 1, format, ##__VA_ARGS__);}
#define LOG_WARN(format, ...) if(0 == LogInstance->m_closeLog) {LogInstance->PrintLog(LogInstance, 2, format, ##__VA_ARGS__);}
#define LOG_ERROR(format, ...) if(0 == LogInstance->m_closeLog) {LogInstance->PrintLog(LogInstance, 3, format, ##__VA_ARGS__);}

#endif