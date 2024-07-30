#include "syspara.h"
#include "def.h"

SysPara syspara = {0};

void SetWorkPath(char *path)
{
    int i, len;
    if(path[0] == '/')
    {
        // 绝对路径
        strcpy(syspara.workPath, path);
    }
    else
    {
        // 相对路径
        if (getcwd(syspara.workPath, PATH_LEN) != NULL) {
            int len = strlen(syspara.workPath);
            path += 1;
            snprintf(&syspara.workPath[len], PATH_LEN, "%s", path);
        }
    }

    len = strlen(syspara.workPath);
    for(i = len;i >= 0;i--)
    {
        if(syspara.workPath[i] == '/')
        {
            syspara.workPath[i] = 0;
            break;
        }
        syspara.workPath[i] = 0;
    }
    printf("%s\n", syspara.workPath);
}
