#include "sqlpool.h"
#include "log.h"

static void Init(struct SqlPool *this, char *dbPath, int poolSize)
{
    int i = 0;
    SqlList *temp = NULL;

    pthread_mutex_init(&this->m_mutex, NULL);

    for(i = 0;i < poolSize;i++)
    {
        temp = malloc(sizeof(SqlList));
        memset(temp, 0, sizeof(SqlList));
        int rc = sqlite3_open(dbPath, &temp->db);
        if(rc) {
            LOG_ERROR("Sqllite init error");
            FREE(temp);
            return;
        }
        this->m_poolSize++;
        if(this->m_frist == NULL){
            this->m_frist = temp;
            this->m_end = temp;
        }
        else{
            this->m_end->next = temp;
            this->m_end = temp;
        }
    }
}

static sqlite3* GetDb(struct SqlPool *this)
{
    pthread_mutex_lock(&this->m_mutex);
    sqlite3 *db = this->m_frist->db;
    SqlList *temp = NULL;
    
    this->m_poolSize--;
    if(this->m_poolSize == 0)
    {
        FREE(this->m_frist);
        this->m_frist = NULL;
        this->m_end = NULL;
    }
    else{
        temp = this->m_frist;
        this->m_frist = this->m_frist->next;
        FREE(temp);
    }
    pthread_mutex_unlock(&this->m_mutex);
    return db;
}
static void FreeDb(struct SqlPool *this, sqlite3* db)
{
    pthread_mutex_lock(&this->m_mutex);
    SqlList *temp = malloc(sizeof(SqlList));
    temp->db = db;
    temp->next = NULL;
    if(this->m_poolSize == 0)
    {
        this->m_frist = temp;
        this->m_end = temp;
    }
    else{
        this->m_end->next = temp;
        this->m_end = temp;
    }
    this->m_poolSize++;
    pthread_mutex_unlock(&this->m_mutex);
}

SqlPool* SqlPoolInit(void)
{
    SqlPool *this = malloc(sizeof(SqlPool));
    memset(this, 0, sizeof(SqlPool));

    this->Init = Init;
    this->GetDb = GetDb;
    this->FreeDb = FreeDb;
    return this;
}

void SqlPoolRelease(SqlPool *this)
{
    SqlList *temp = this->m_frist;
    while(this->m_frist != NULL)
    {
        this->m_frist = temp->next;
        sqlite3_close(temp->db);
        FREE(temp);
        this->m_poolSize--;
        temp = this->m_frist;
    }
    pthread_mutex_destroy(&this->m_mutex);
    FREE(this);
}