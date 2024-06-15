#include "threadpool.h"
#include "webserver.h"
#include "syspara.h"

static void* worker(void *arg)
{
    WebServer *server = ParaCheck(server);
    ThreadPool *this = server->m_thpool;

    while(true)
    {
        pthread_mutex_lock(&this->m_mutex);
        while (this->m_frist == NULL && !this->m_shutdown)
        {
            pthread_cond_wait(&this->m_cond, &this->m_mutex);
        }
        if(this->m_shutdown)
        {
            pthread_mutex_unlock(&this->m_mutex);
            pthread_exit(NULL);
        }
        Task *t = this->m_frist;
        this->m_frist = t->next;
        this->m_taskSize--;
        pthread_mutex_unlock(&this->m_mutex);
        t->run(t->arg);
        FREE(t);
        t = NULL; 
    }
}

static void Init(struct ThreadPool *this, int num)
{
    this->m_taskNum = num;
    pthread_mutex_init(&this->m_mutex, NULL);
    pthread_cond_init(&this->m_cond, NULL);

    int i = 0;
    for (i = 0; i < num; i++)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, worker, this);
    }
}

static void AddTask(struct ThreadPool *this, void(*run)(void*), void *arg)
{
    Task *t = malloc(sizeof(Task));
    memset(t, 0, sizeof(Task));
    t->arg = arg;
    t->run = run;

    pthread_mutex_lock(&this->m_mutex);
    if(this->m_frist==NULL)//第一个任务
    {
        this->m_frist = t;
        this->m_end = t;
    }
    else
    {
        this->m_end->next = t;
        this->m_end = t;
    }
    this->m_taskSize++;
    pthread_cond_signal(&this->m_cond);
    pthread_mutex_unlock(&this->m_mutex);
}

ThreadPool* ThreadPoolInit(void)
{
    ThreadPool *this = malloc(sizeof(ThreadPool));
    memset(this, 0, sizeof(ThreadPool));

    this->Init = Init;
    this->AddTask = AddTask;
    return this;
}
void ThreadPoolRelease(ThreadPool *this)
{
    this->m_shutdown = 1;
    int i = 0;
    for(i=0;i<this->m_taskNum;i++)
    {
        pthread_cond_signal(&this->m_cond);
    }
    pthread_mutex_destroy(&this->m_mutex);
    pthread_cond_destroy(&this->m_cond);
    FREE(this);
}