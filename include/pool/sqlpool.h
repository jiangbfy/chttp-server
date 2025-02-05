#ifndef SQLPOOL_H
#define SQLPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sqlite3.h"
#include "def.h"

typedef struct SqlPool
{
    // Sqlite数据库指针
    sqlite3 *db;
    // 互斥锁
    pthread_mutex_t m_mutex;

    // 数据库连接初始化
    bool (*Init)(struct SqlPool *this, char *dbPath);
    // 获取一个连接
    sqlite3* (*GetDb)(struct SqlPool *this);
    // 释放一个连接
    void (*FreeDb)(struct SqlPool *this);
} SqlPool;

SqlPool* SqlPoolInit(void);
void SqlPoolRelease(SqlPool *this);

#endif