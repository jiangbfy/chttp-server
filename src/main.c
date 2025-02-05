#include "webserver.h"
#include "syspara.h"
#include "log.h"
#include "map.h"

int main(int argc, char *argv[])
{
    //日志打印初始化
    SetWorkPath(argv[0]);
    LogInit();

    WebServer *server = WebServerInit();
    ParaCommit(server);
    server->Config(server, 9000, "server.db", 8);
    server->Init(server);
    server->Run(server);
    WebServerRelease(server);
    LogRelease();

    return 0;
}