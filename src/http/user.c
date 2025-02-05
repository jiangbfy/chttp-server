#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"
#include "cJSON.h"
#include "log.h"
#include "webserver.h"
#include "sqlpool.h"
#include "syspara.h"

void All(Http *http)
{
    WebServer *server = ParaCheck(server);
    SqlPool *pool = server->m_sqlpool;
    sqlite3* db = pool->GetDb(pool);
    int r = 0,c = 0;
    char **table = NULL;
    char *errMsg;
    char *sql = "SELECT * FROM user;";
    int ret = sqlite3_get_table(db, sql, &table, &r, &c, &errMsg);
    if(ret != SQLITE_OK) {
        sqlite3_free(errMsg);
    }
    http->m_respBody = malloc(1024);
    char *pbuf = http->m_respBody;
    int len = sprintf(pbuf, "{\"code\": 200, \"message\": \"usccess\", \"data\": [");
    pbuf += len;
    for(int i = 0;i < r;i++)
    {
        len = sprintf(pbuf, "{\"id\": \"%s\", \"user\": \"%s\", \"passwd\": \"%s\", \"time\": \"%s\"},", table[4 + i*4], table[5 + i*4], table[6 + i*4], table[7 + i*4]);
        pbuf += len;
    }
    len = sprintf(pbuf, "]}");
    sqlite3_free_table(table);
    pool->FreeDb(pool);
    pbuf += len;
    http->m_respBodyLen = pbuf - http->m_respBody;
}

const Service UserService[] = {
    {"/all", METHOD_GET, All},
    {"NULL", METHOD_NULL, NULL}
};
