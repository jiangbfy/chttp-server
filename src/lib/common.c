#include "common.h"

char* searchStrPos(char *pbuf, char *str, int limit)
{
    int len = strlen(str);
    for (int i = 0;i < limit;i++)
    {
        if (*pbuf == *str)
        {
            if (memcmp(pbuf, str, len) == 0) return pbuf;
        }
        pbuf++;
    }
    return NULL;
}

char* searchStrNeg(char *pbuf, char *str, int limit)
{
    int len = strlen(str);
    for (int i = 0;i < limit;i++)
    {
        if (*pbuf == *str)
        {
            if (memcmp(pbuf, str, len) == 0) return pbuf;
        }
        pbuf--;
    }
    return NULL;
}
