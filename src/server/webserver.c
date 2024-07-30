#include "webserver.h"
#include "syspara.h"
#include "log.h"
#include "http.h"

static void Config(struct WebServer *this, int port, char *user, char *pwd, char *dbName, int taskNum)
{
    this->m_port = port;
    this->m_user = malloc(strlen(user) + 1);
    strcpy(this->m_user, user);
    this->m_pwd = malloc(strlen(pwd) + 1);
    strcpy(this->m_pwd, pwd);
    this->m_dbName = malloc(strlen(dbName) + 1);
    strcpy(this->m_dbName, dbName);
    this->m_taskNum = taskNum;
}

//对文件描述符设置非阻塞
static int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
static void addfd(int epollfd, int fd, int one_shot, int trigMode)
{
    struct epoll_event event;
    event.data.fd = fd;

    if (1 == trigMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

static void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}
//向管道发送消息，TIMEOUT一次
static void alarm_handler(int sig)
{
    int msg = sig;
    WebServer *server = ParaCheck(server);
    send(server->m_pipefd[1], (char *)&msg, 1, 0);
    alarm(TIMEOUT);
}
//接收向管道消息，置标志
static void deal_signal(struct WebServer *this, int sockfd)
{
    int msg = 0;
    recv(sockfd, (char *)&msg, sizeof(msg), 0);
    this->isTimeout = true;
}
//对超时的fd压栈保存
static void StackPush(int id)
{
    WebServer *server = ParaCheck(server);
    server->m_stack[server->m_stackSize] = id;
    server->m_stackSize++;
}
//对超时的fd出栈处理
static int StackPop(void)
{
    WebServer *server = ParaCheck(server);
    server->m_stackSize--;
    return server->m_stack[server->m_stackSize];
}
//tcp连接超时检查
static void ExpireCheck(TreeNode* node, WsList *temp) {
    if (node != NULL) {
        ExpireCheck(node->left, temp);
        Tcp *conn = (Tcp *)node->data;
        conn->m_expire -= TIMEOUT;
        if(conn->m_expire < 0)
        {
            StackPush(node->id);
        }
        else
        {
            if(conn->m_ws == WS_NORMAL)
            {
                if(temp != NULL)
                {
                    conn->m_txBuf[0] = 0x81;
                    if(temp->len < 126)
                    {
                        conn->m_txBuf[1] = temp->len;
                        strcpy(&conn->m_txBuf[2], temp->msg);
                        LOG_INFO("TxSocket: %s", &conn->m_txBuf[2]);
                    }
                    else if(temp->len >= 126 && temp->len < (BUF_LEN - 4))
                    {
                        conn->m_txBuf[1] = 126;
                        conn->m_txBuf[2] = (temp->len >> 8);
                        conn->m_txBuf[3] = temp->len;
                        strcpy(&conn->m_txBuf[4], temp->msg);
                        if(temp->len < LOG_LEN) LOG_INFO("TxSocket: %s", &conn->m_txBuf[4]);
                    }
                    modfd(conn->m_epollfd, conn->m_sockfd, EPOLLOUT, true, 0);
                }
            }
        }
        ExpireCheck(node->right, temp);
    }

}

static void PushMsg(struct WebServer *this, char *msg, uint16_t len)
{
    WsList *temp = malloc(sizeof(WsList));
    temp->msg = malloc(len + 1);
    strcpy(temp->msg, msg);
    temp->len = len;

    pthread_mutex_lock(&this->m_mutex);
    this->m_wsSize++;
    if(this->m_frist == NULL){
        this->m_frist = temp;
        this->m_end = temp;
    }
    else{
        this->m_end->next = temp;
        this->m_end = temp;
    }
    pthread_mutex_unlock(&this->m_mutex);
    alarm(1);
}

static WsList* PopMsg(struct WebServer *this)
{
    WsList *temp = NULL;
    if(this->m_wsSize > 1)
    {
        pthread_mutex_lock(&this->m_mutex);
        temp = this->m_frist;
        this->m_frist = this->m_frist->next;
        this->m_wsSize--;
        pthread_mutex_unlock(&this->m_mutex);
    }
    else if(this->m_wsSize == 1)
    {
        pthread_mutex_lock(&this->m_mutex);
        temp = this->m_frist;
        this->m_frist = NULL;
        this->m_end = NULL;
        this->m_wsSize--;
        pthread_mutex_unlock(&this->m_mutex);
    }
    return temp;
}

static void Init(struct WebServer *this)
{
    this->m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    // 0, 1, 2的是给标准输入输出用的
    ASSERT(this->m_listenfd >= 3);

    // 对方关闭连接后，一段时间后再关闭
    struct linger tmp = {1, 1};
    setsockopt(this->m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    // 允许端口被立即重复使用
    int flag = 1;
    setsockopt(this->m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(this->m_port);
    ret = bind(this->m_listenfd, (struct sockaddr *)&address, sizeof(address));
    ASSERT(ret == 0);
    ret = listen(this->m_listenfd, 5);
    ASSERT(ret == 0);
    LOG_INFO("Server port: %d", this->m_port);

    this->m_epollfd = epoll_create(5);
    ASSERT(this->m_epollfd >= 3);
    addfd(this->m_epollfd, this->m_listenfd, false, 0);
    //创建管道
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, this->m_pipefd);
    ASSERT(ret != -1);
    setnonblocking(this->m_pipefd[1]);
    addfd(this->m_epollfd, this->m_pipefd[0], false, 0);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, alarm_handler);
    alarm(TIMEOUT);

    this->m_thpool->Init(this->m_thpool, this->m_taskNum);
    LOG_INFO("Threadpool num: %d", this->m_taskNum);

    char dbPath[PATH_LEN];
    char *workPath = ParaCheck(workPath);
    int len = strlen(workPath);
    memcpy(dbPath, workPath, len);
    sprintf(&dbPath[len], "/%s.db", this->m_dbName);
    LOG_INFO("Database path: %s", dbPath);
    this->m_sqlpool->Init(this->m_sqlpool, dbPath, 1);
    LOG_INFO("Sqlpool num: 1");
    SqlPool *sqlpool = this->m_sqlpool;
    ParaCommit(sqlpool);
}

static void AddConn(struct WebServer *this, int clientfd, struct sockaddr_in *addr)
{
    addfd(this->m_epollfd, clientfd, true, 0);
    Tcp *conn = TcpInit();
    conn->Config(conn, this->m_epollfd, clientfd, addr, EXPIRE);
    this->m_tree->Insert(this->m_tree, clientfd, (void *)conn);
    LOG_INFO("New connection fd: %d ip: %d port: %d", clientfd, addr->sin_addr.s_addr, addr->sin_port);
}

static void ReadMsg(struct WebServer *this, int sockfd)
{
    TreeNode *node = this->m_tree->Search(this->m_tree, sockfd);
    Tcp *conn = (Tcp *)node->data;
    conn->m_expire = EXPIRE;
    this->m_thpool->AddTask(this->m_thpool, (void*)conn->Recv, conn);
    LOG_INFO("Recv from fd: %d ip: %d port: %d", sockfd, conn->m_address.sin_addr.s_addr, conn->m_address.sin_port);
}

static void SendMsg(struct WebServer *this, int sockfd)
{
    TreeNode *node = this->m_tree->Search(this->m_tree, sockfd);
    Tcp *conn = (Tcp *)node->data;
    conn->m_expire = EXPIRE;
    this->m_thpool->AddTask(this->m_thpool, (void*)conn->Send, conn);
    LOG_INFO("Send to fd: %d ip: %d port: %d", sockfd, conn->m_address.sin_addr.s_addr, conn->m_address.sin_port);
}

static void CloseConn(struct WebServer *this, int sockfd)
{
    removefd(this->m_epollfd, sockfd);
    close(sockfd);
    this->m_tree->Delete(this->m_tree, sockfd);
    LOG_INFO("Close fd: %d", sockfd);
}

static void Run(struct WebServer *this)
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int number, sockfd, i = 0;

    while(true)
    {
        number = epoll_wait(this->m_epollfd, this->m_events, EPOLL_SIZE, -1);
        for (i = 0; i < number; i++)
        {
            sockfd = this->m_events[i].data.fd;
            //新连接接入
            if (sockfd == this->m_listenfd)
            {
                int clientfd = accept(this->m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                AddConn(this, clientfd, &client_address);
            }
            //管道信息
            else if ((sockfd == this->m_pipefd[0]) && (this->m_events[i].events & EPOLLIN))
            {
                deal_signal(this, sockfd);
            }
            //关闭连接
            else if (this->m_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                CloseConn(this, sockfd);
            }
            //接收数据
            else if (this->m_events[i].events & EPOLLIN)
            {
                ReadMsg(this, sockfd);
            }
            //发送数据
            else if (this->m_events[i].events & EPOLLOUT)
            {
                SendMsg(this, sockfd);
            }
        }
        //处理超时连接
        if(this->isTimeout)
        {
            this->isTimeout = false;
            WsList *temp = this->PopMsg(this);
            ExpireCheck(this->m_tree->m_root, temp);
            if(temp != NULL)
            {
                FREE(temp->msg);
                FREE(temp);
            }
            while(this->m_stackSize > 0)
            {
                sockfd = StackPop();
                CloseConn(this, sockfd);
            }
            if(this->m_wsSize > 0) alarm(1);
        }
    }
}

WebServer* WebServerInit(void)
{
    WebServer *this = malloc(sizeof(WebServer));
    memset(this, 0, sizeof(WebServer));
    pthread_mutex_init(&this->m_mutex, NULL);

    this->m_tree = TreeInit();
    this->m_thpool = ThreadPoolInit();
    this->m_sqlpool = SqlPoolInit();
    this->Config = Config;
    this->Init = Init;
    this->Run = Run;
    this->PushMsg = PushMsg;
    this->PopMsg = PopMsg;
    return this;
}

void WebServerRelease(WebServer* this)
{
    close(this->m_listenfd);
    close(this->m_epollfd);
    FREE(this->m_user);
    FREE(this->m_pwd);
    FREE(this->m_dbName);
    TreeRelease(this->m_tree);
    ThreadPoolRelease(this->m_thpool);
    SqlPoolRelease(this->m_sqlpool);
    pthread_mutex_destroy(&this->m_mutex);
    FREE(this);
}
