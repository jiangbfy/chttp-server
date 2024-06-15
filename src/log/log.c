#include <sys/time.h>
#include "log.h"
#include "syspara.h"
#include "def.h"
#include <unistd.h>

Log *LogInstance;

void AddMsg(struct Log *this, char *msg, int len)
{
    MsgList *temp = malloc(sizeof(MsgList));
    temp->msg = malloc(len + 1);
    strcpy(temp->msg, msg);

    pthread_mutex_lock(&this->m_mutex);
    this->m_size++;
    if(this->m_frist == NULL){
        this->m_frist = temp;
        this->m_end = temp;
    }
    else{
        this->m_end->next = temp;
        this->m_end = temp;
    }
    pthread_mutex_unlock(&this->m_mutex);
}

void PrintLog(struct Log *this, int level, const char *format, ...)
{
    //32字节留给Log信息头
    char *buffer = malloc(LOG_LEN + 32);
    char *pbuf = buffer;
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    //Log信息头
    int len = sprintf(pbuf, "%02d-%02d-%02d %02d:%02d:%02d ", sys_tm->tm_year - 100, sys_tm->tm_mon + 1, sys_tm->tm_mday, sys_tm->tm_hour, sys_tm->tm_min, sys_tm->tm_sec);
    pbuf += len;
    switch (level)
    {
    case 0:
        strcpy(pbuf, "[debug]: ");
        pbuf += 9;
        break;
    case 1:
        strcpy(pbuf, "[info]: ");
        pbuf += 8;
        break;
    case 2:
        strcpy(pbuf, "[warn]: ");
        pbuf += 8;
        break;
    case 3:
        strcpy(pbuf, "[erro]: ");
        pbuf += 8;
        break;
    default:
        strcpy(pbuf, "[info]: ");
        pbuf += 8;
        break;
    }

    if(this->m_today != sys_tm->tm_mday)
    {
        //日期发生变更，新建文件
        pthread_mutex_lock(&this->m_mutex);
        this->m_today = sys_tm->tm_mday;
        char logFile[32];
        if(this->m_fp != NULL)
        {
            fflush(this->m_fp);
            fclose(this->m_fp);
            this->m_fp = NULL;
        }
        sprintf(logFile, "%02d_%02d_%02d", sys_tm->tm_year - 100, sys_tm->tm_mon + 1, sys_tm->tm_mday);
        SetLogPath(logFile);
        char *logPath = ParaCheck(logPath);
        this->m_fp = fopen(logPath, "a");
        pthread_mutex_unlock(&this->m_mutex);
    }

    va_list argList;
    va_start(argList, format);
    len = vsprintf(pbuf, format, argList);
    va_end(argList);
    pbuf += len;
    *pbuf = '\n';
    pbuf++;
    *pbuf = 0;
    len = pbuf - buffer;

    this->AddMsg(this, buffer, len);
    FREE(buffer);
}

static void* WriteLogThread(void *arg)
{
    struct Log *this = (Log *)arg;
    MsgList *temp = NULL;
    while(this->m_shutdown == 0)
    {
        if(this->m_size > 1)
        {
            pthread_mutex_lock(&this->m_mutex);
            temp = this->m_frist;
            this->m_frist = this->m_frist->next;
            this->m_size--;
            fputs(temp->msg, this->m_fp);
            fflush(this->m_fp);
            FREE(temp->msg);
            FREE(temp);
            pthread_mutex_unlock(&this->m_mutex);
        }
        else if(this->m_size == 1)
        {
            pthread_mutex_lock(&this->m_mutex);
            temp = this->m_frist;
            this->m_frist = NULL;
            this->m_end = NULL;
            this->m_size--;
            fputs(temp->msg, this->m_fp);
            fflush(this->m_fp);
            FREE(temp->msg);
            FREE(temp);
            pthread_mutex_unlock(&this->m_mutex);
        }
        else
        {
            sleep(1);
        }
    }
    return NULL;
}

void LogInit(void)
{
    LogInstance = malloc(sizeof(Log));
    memset(LogInstance, 0, sizeof(Log));
    pthread_mutex_init(&LogInstance->m_mutex, NULL);

    LogInstance->AddMsg = AddMsg;
    LogInstance->PrintLog = PrintLog;
    pthread_t tid;
    pthread_create(&tid, NULL, WriteLogThread, LogInstance);
}

void LogRelease(void)
{
    MsgList *temp = LogInstance->m_frist;
    while(LogInstance->m_frist != NULL)
    {
        LogInstance->m_frist = temp->next;
        FREE(temp->msg);
        FREE(temp);
        temp = LogInstance->m_frist;
    }
    LogInstance->m_shutdown = 1;
    pthread_mutex_destroy(&LogInstance->m_mutex);
}