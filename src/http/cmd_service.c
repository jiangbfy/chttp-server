#include <stdio.h>
#include <string.h>
#include "cmd_service.h"
#include "cJSON.h"
#include "log.h"

int Uname(char *argOut, Http *this)
{
    char type[32], host[32], arch[32], version[32], ch;
    int cnt = 0, len = 0;
    //系统类型
    FILE *fp = popen("uname -s", "r");
    while((ch = fgetc(fp)) != -1)
	{
		type[cnt] = ch;
        cnt++;
	}
    type[cnt -1] = 0;
    pclose(fp);
    cnt = 0;
    //主机名
    fp = popen("uname -n", "r");
    while((ch = fgetc(fp)) != -1)
	{
		host[cnt] = ch;
        cnt++;
	}
    host[cnt -1] = 0;
    pclose(fp);
    cnt = 0;
    //硬件架构
    fp = popen("uname -m", "r");
    while((ch = fgetc(fp)) != -1)
	{
		arch[cnt] = ch;
        cnt++;
	}
    arch[cnt -1] = 0;
    pclose(fp);
    cnt = 0;
    //内核版本
    fp = popen("uname -r", "r");
    while((ch = fgetc(fp)) != -1)
	{
		version[cnt] = ch;
        cnt++;
	}
    version[cnt -1] = 0;
    pclose(fp);

    len = sprintf(argOut, "{\"type\":\"%s\",\"host\":\"%s\",\"arch\":\"%s\",\"version\":\"%s\"}", type, host, arch, version);
    return len;
}

const Service CmdServiceList[] = {
    {"/uname", Uname},
    {"NULL", NULL}
};