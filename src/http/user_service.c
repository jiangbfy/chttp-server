#include <stdio.h>
#include <string.h>
#include "syspara.h"
#include "cmd_service.h"
#include "cJSON.h"
#include "sqlite3.h"
#include "sqlpool.h"
#include "log.h"

int UserInfo(char *argOut, Http *this)
{
    SqlPool *pool = ParaCheck(sqlpool);
    sqlite3 *db = pool->GetDb(pool);

    int r=0,c=0;
    char **table=NULL;
    char *pbuf = argOut;
    char *errMsg;
    char *sql = "SELECT * FROM userinfo;";
    LOG_INFO("SQL: %s", sql);
    int ret = sqlite3_get_table(db, sql, &table, &r, &c, &errMsg);
    if(ret != SQLITE_OK)
    {
        LOG_ERROR("%s", errMsg);
        pool->FreeDb(pool, db);
        sqlite3_free(errMsg);
        return 0;
    }
    if(r > 0)
    {
        int i = 0, len;
        *pbuf = '[';
        pbuf++;
        for(i = 0;i < r;i++)
        {
            len = sprintf(pbuf, "{\"id\":\"%s\",\"user\":\"%s\",\"passwd\":\"%s\",\"time\":\"%s\"},", table[4 + i*4], table[5 + i*4], table[6 + i*4], table[7 + i*4]);
            pbuf += len;
        }
        *(pbuf - 1) = ']';
        *pbuf = 0;
    }
    sqlite3_free_table(table);
    pool->FreeDb(pool, db);
    return (pbuf - argOut);
}

int Adduser(char *argOut, Http *this)
{
    int len = 0;
    cJSON* object = cJSON_CreateObject();
    object = cJSON_Parse(this->m_body);
    char* user = NULL;
    if(cJSON_HasObjectItem(object, "user")) user = cJSON_GetObjectItem(object, "user")->valuestring;
    char* passwd = NULL;
    if(cJSON_HasObjectItem(object, "passwd")) passwd = cJSON_GetObjectItem(object, "passwd")->valuestring;
    char* time = NULL;
    if(cJSON_HasObjectItem(object, "time")) time = cJSON_GetObjectItem(object, "time")->valuestring;

    if(user == NULL || passwd == NULL || time == NULL)
    {
        len = sprintf(argOut, "{\"msg\":\"fail 1\"}");
        cJSON_Delete(object);
        return len;
    }
    if(*user == 0 || *passwd == 0 || *time == 0)
    {
        len = sprintf(argOut, "{\"msg\":\"fail 2\"}");
        cJSON_Delete(object);
        return len;
    }

    SqlPool *pool = ParaCheck(sqlpool);
    sqlite3 *db = pool->GetDb(pool);
    char sql[256];
    char *errMsg;
    sprintf(sql, "INSERT INTO userinfo (user, passwd, time) VALUES ('%s', '%s', '%s');", user, passwd, time);
    LOG_INFO("SQL: %s", sql);
    int rc = sqlite3_exec(db, sql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        LOG_ERROR("%s", errMsg);
        sqlite3_free(errMsg);
    }
    len = sprintf(argOut, "{\"msg\":\"success\"}");
    cJSON_Delete(object);
    pool->FreeDb(pool, db);
    return len;
}

const Service UserServiceList[] = {
    {"/userinfor", UserInfo},
    {"/adduser", Adduser},
    {"NULL", NULL}
};