#include "sqlpool.h"
#include "syspara.h"
#include "log.h"

static bool Init(struct SqlPool *this, char *dbNmae)
{
    pthread_mutex_init(&this->m_mutex, NULL);
    char dbPath[520];
    char *workPath = ParaCheck(workPath);
    sprintf(dbPath, "%s/%s", workPath, dbNmae);

    int rc = sqlite3_open(dbPath, &this->db);
    if(rc) {
        LOG_ERROR("Sqlite init error");
        return false;
    }
    return true;
}

static sqlite3* GetDb(struct SqlPool *this)
{
    pthread_mutex_lock(&this->m_mutex);
    return this->db;
}
static void FreeDb(struct SqlPool *this)
{
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
    sqlite3_close(this->db);
    pthread_mutex_destroy(&this->m_mutex);
    FREE(this);
}