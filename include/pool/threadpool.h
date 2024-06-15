#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "def.h"

typedef struct Task
{
    // 要执行的函数的函数指针
    void (*run)(void *arg);
    // 函数参数
    void *arg;
    // 下一个节点
    struct Task *next;
} Task;
//线程池
typedef struct ThreadPool
{
    // 用链表存储任务列表，m_frist是链表头，m_end是链表尾
    Task *m_frist;
    Task *m_end;
    //线程数量
    int m_taskNum;
    //任务数量
    int m_taskSize;
    //互斥量
    pthread_mutex_t m_mutex;
    //条件变量
    pthread_cond_t m_cond;
    //线程退出标志
    int m_shutdown;
    //初始化 (线程池数量)
    void (*Init)(struct ThreadPool *this, int num);
    //添加任务 (要执行的函数的函数指针, 函数参数)
    void (*AddTask)(struct ThreadPool *this, void(*run)(void*), void *arg);
} ThreadPool;

ThreadPool* ThreadPoolInit(void);
void ThreadPoolRelease(ThreadPool *this);

#endif
