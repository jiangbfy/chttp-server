#include "controller.h"
#include "cmd_service.h"
#include "user_service.h"
#include "ws_service.h"

const static Controller ControllerList[] = {
    {"/cmd", CmdServiceList},
    {"/user", UserServiceList},
    {"/api", WsServiceList},
    {"NULL", NULL}
};

int ControllerParse(char *argOut, Http *this)
{
    char *url = this->m_url;
    Controller *con = (Controller *)ControllerList;
    Service *serv = NULL;
    int i = 0, fristUrlLen = 0, secondUrlLen = 0, rtnLen = 0;
    while(con->m_service != NULL)
    {
        fristUrlLen = strlen(con->m_fristUrl);
        if(memcmp(con->m_fristUrl, url, fristUrlLen) == 0)
        {
            serv = (Service *)con->m_service;
            break;
        }
        con++;
    }
    if(con[i].m_service == NULL) return -1;

    url += fristUrlLen;
    while(serv[i].ParseReq != NULL)
    {
        secondUrlLen = strlen(serv[i].m_SecondUrl);
        if(memcmp(serv[i].m_SecondUrl, url, secondUrlLen) == 0)
        {
            rtnLen = serv[i].ParseReq(argOut, this);
            break;
        }
        i++;
    }
    if(serv[i].ParseReq == NULL) return -1;

    return rtnLen;
}
