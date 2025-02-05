#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"
#include "cJSON.h"
#include "log.h"

void Hello(Http *http)
{
    http->m_respBody = malloc(512);
    char *value = http->m_param->Search(http->m_param, "name");
    http->m_respBodyLen =  sprintf(http->m_respBody, "{\"name\":%s}", value);
}

const Service CmdService[] = {
    {"/hello", METHOD_GET, Hello},
    {"NULL", METHOD_NULL, NULL}
};
