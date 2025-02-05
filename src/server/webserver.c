#include "webserver.h"
#include "syspara.h"
#include "log.h"

static void Config(struct WebServer *this, int port, char *dbName, int taskNum)
{
    this->m_port = port;
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

    this->m_sqlpool->Init(this->m_sqlpool, this->m_dbName);
}

static void AddConn(struct WebServer *this, int clientfd, struct sockaddr_in *addr)
{
    addfd(this->m_epollfd, clientfd, true, 0);
    Tcp *conn = TcpInit();
    conn->Config(conn, this->m_epollfd, clientfd, addr);
    this->m_list->Push(this->m_list, clientfd, (void *)conn);
    LOG_INFO("New connection fd: %d ip: %d port: %d", clientfd, addr->sin_addr.s_addr, addr->sin_port);
}

static void ReadMsg(struct WebServer *this, int sockfd)
{
    ListNode *node = this->m_list->Find(this->m_list, sockfd);
    Tcp *conn = (Tcp *)node->data;
    this->m_thpool->AddTask(this->m_thpool, (void*)conn->Recv, conn);
}

static void SendMsg(struct WebServer *this, int sockfd)
{
    ListNode *node = this->m_list->Find(this->m_list, sockfd);
    Tcp *conn = (Tcp *)node->data;
    this->m_thpool->AddTask(this->m_thpool, (void*)conn->Send, conn);
}

static void CloseConn(struct WebServer *this, int sockfd)
{
    removefd(this->m_epollfd, sockfd);
    close(sockfd);
    this->m_list->Pop(this->m_list, sockfd);
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
            this->m_list->Check(this->m_list, TIMEOUT);
        }
    }
}

void Push(struct List *this, int id, void *data)
{
    if (this->m_size >= TCP_SIZE) return;

    ListNode *node = malloc(sizeof(ListNode));
    memset(node, 0, sizeof(ListNode));
    node->id = id;
    node->timeout = EXPIRE;
    node->data = data;

    if (this->m_size == 0)
    {
        this->m_head = node;
        this->m_end = node;
    }
    else
    {

        this->m_end->next = node;
        node->prev = this->m_end;
        this->m_end = node;
    }
    this->m_size++;
}

void Pop(struct List *this, int id)
{
    ListNode *temp = this->m_head;
    ListNode *prev = NULL, *next = NULL;
    while (temp != NULL)
    {
        if (temp->id == id)
        {
            if(this->m_size == 1)
            {
                this->m_head = NULL;
                this->m_end = NULL;
                this->m_size--;
                FREE(temp->data);
                FREE(temp);
                return;
            }
            if(temp == this->m_head)
            {
                this->m_head = temp->next;
                this->m_head->prev = NULL;
            }
            else if(temp == this->m_end)
            {
                prev = temp->prev;
                this->m_end = prev;
                this->m_end->next = NULL;
            }
            else
            {
                prev = temp->prev;
                next = temp->next;
                prev->next = next;
                next->prev = prev;
            }
            this->m_size--;
            FREE(temp->data);
            FREE(temp);
            return;
        }
        temp = temp->next;
    }
}

ListNode* Find(struct List *this, int id)
{
    ListNode *temp = this->m_head;
    ListNode *prev = NULL, *next = NULL;
    while (temp != NULL)
    {
        if (temp->id == id)
        {
            if(this->m_size == 1)
            {
                temp->timeout = EXPIRE;
                return temp;
            }
            if(temp == this->m_head)
            {
                this->m_head = temp->next;
                this->m_head->prev = NULL;
                temp->timeout = EXPIRE;
                temp->next = NULL;
                this->m_end->next = temp;
                temp->prev = this->m_end;
                this->m_end = temp;
            }
            else if(temp == this->m_end)
            {
                temp->timeout = EXPIRE;
            }
            else
            {
                prev = temp->prev;
                next = temp->next;
                prev->next = next;
                next->prev = prev;
                temp->next = NULL;
                temp->timeout = EXPIRE;
                this->m_end->next = temp;
                temp->prev = this->m_end;
                this->m_end = temp;
            }
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

void Check(struct List *this, int time)
{
    ListNode *temp = this->m_head;
    ListNode *prev = NULL, *next = NULL;
    WebServer *server = ParaCheck(server);
    while (temp != NULL)
    {
        temp->timeout -= time;
        if (temp->timeout <= 0)
        {
            if(this->m_size == 1)
            {
                LOG_INFO("release: %d", temp->id);
                this->m_head = NULL;
                this->m_end = NULL;
                this->m_size--;
                removefd(server->m_epollfd, temp->id);
                close(temp->id);
                FREE(temp->data);
                FREE(temp);
                return;
            }
            if(temp == this->m_head)
            {
                this->m_head = temp->next;
                this->m_head->prev = NULL;
            }
            else if (temp == this->m_end)
            {
                this->m_end = temp->prev;
                this->m_end->next = NULL;
            }
            else
            {
                prev = temp->prev;
                next = temp->next;
                prev->next = next;
                next->prev = prev;
            }
            LOG_INFO("release: %d", temp->id);
            removefd(server->m_epollfd, temp->id);
            close(temp->id);
            FREE(temp->data);
            FREE(temp);
            this->m_size--;
        }
        else
        {
            temp = temp->next;
        }
    }
}

List* ListInit(void)
{
    List *this = malloc(sizeof(List));
    memset(this, 0, sizeof(List));

    this->Push = Push;
    this->Find = Find;
    this->Pop = Pop;
    this->Check = Check;
    return this;
}

void ListRelease(List* this)
{
    while (this->m_size > 0)
    {
        ListNode *next = this->m_head->next;
        FREE(this->m_head->data);
        FREE(this->m_head);
        this->m_head = next;
        this->m_size--;
    }
}

WebServer* WebServerInit(void)
{
    WebServer *this = malloc(sizeof(WebServer));
    memset(this, 0, sizeof(WebServer));
    pthread_mutex_init(&this->m_mutex, NULL);

    this->m_list = ListInit();
    this->m_thpool = ThreadPoolInit();
    this->m_sqlpool = SqlPoolInit();
    this->Config = Config;
    this->Init = Init;
    this->Run = Run;
    return this;
}

void WebServerRelease(WebServer* this)
{
    close(this->m_listenfd);
    close(this->m_epollfd);
    FREE(this->m_dbName);
    ListRelease(this->m_list);
    ThreadPoolRelease(this->m_thpool);
    SqlPoolRelease(this->m_sqlpool);
    pthread_mutex_destroy(&this->m_mutex);
    FREE(this);
}
