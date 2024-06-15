#include "webserver.h"
#include "syspara.h"
#include "log.h"

int main(int argc, char *argv[])
{
    //日志打印初始化
    SetWorkPath(argv[0]);
    LogInit();
    WebServer *server = WebServerInit();
    ParaCommit(server);
    server->Config(server, 16790, "root", "root", "server", 8);
    server->Init(server);
    server->Run(server);
    WebServerRelease(server);
    LogRelease();

    return 0;
}