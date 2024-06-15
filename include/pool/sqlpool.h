#ifndef SQLPOOL_H
#define SQLPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sqlite3.h"
#include "def.h"

typedef struct SqlList
{
    // Sqlite数据库指针
    sqlite3 *db;
    // 下一个节点
    struct SqlList *next;
} SqlList;

typedef struct SqlPool
{
    // 数据库连接池大小
    int m_poolSize;
    // 用链表存储数据库连接池，m_frist是链表头，m_end是链表尾
    SqlList *m_frist;
    SqlList *m_end;
    // 互斥锁
    pthread_mutex_t m_mutex;

    // 数据库连接池初始化 (数据库路径，数据库连接池大小)
    void (*Init)(struct SqlPool *this, char *dbPath, int poolSize);
    // 从数据库连接池中获取一个连接
    sqlite3* (*GetDb)(struct SqlPool *this);
    // 把连接放回到数据库连接池中
    void (*FreeDb)(struct SqlPool *this, sqlite3* db);
} SqlPool;

SqlPool* SqlPoolInit(void);
void SqlPoolRelease(SqlPool *this);

#endif