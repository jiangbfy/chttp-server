#include "tcp.h"
#include "log.h"
#include "webserver.h"
#include "syspara.h"

void modfd(int epollfd, int fd, int ev, int one_shot, int trigMode)
{
    struct epoll_event event;
    event.data.fd = fd;

    if (1 == trigMode)
        event.events = ev | EPOLLET | EPOLLRDHUP;
    else
        event.events = ev | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

static void Process(struct Tcp *this)
{
    if(this->m_ws == WS_NULL)
    {
        Http *http = HttpInit();
        http->m_address = &this->m_address;
        http->ProcessRequest(http, this->m_rxBuf, this->m_rxLen);
        http->ProcessResponse(http, this->m_txBuf, this->m_txHead);
        this->m_ws = http->m_ws;
        HttpRelease(http);
        modfd(this->m_epollfd, this->m_sockfd, EPOLLOUT, true, 0);
    }
    else if(this->m_ws == WS_NORMAL)
    {
        this->m_rxBuf[this->m_rxLen] = 0;
        uint8_t len = this->m_rxBuf[1] & 0x7F;
        char *mask = NULL, *msg = NULL;
        int i = 0;
        uint8_t id = 0;
        if(len < 126) {mask = &this->m_rxBuf[2]; i = 6;}
        else if(len == 126) {mask = &this->m_rxBuf[4]; i = 8;}
        else {mask = &this->m_rxBuf[10]; i = 14;}
        msg = &this->m_rxBuf[i];
        for(;i < this->m_rxLen;i++)
        {
            this->m_rxBuf[i] ^= mask[id];
            id = (id + 1) & 0x03;
        }
        LOG_INFO("RxSocket: %s", msg);
        modfd(this->m_epollfd, this->m_sockfd, EPOLLIN, true, 0);
        LOG_INFO("====================================================================================================");
    }
    else
    {
        this->m_rxBuf[this->m_rxLen] = 0;
        LOG_INFO("RxPublish: %s", this->m_rxBuf);
        WebServer *server;
        server = ParaCheck(server);
        server->PushMsg(server, this->m_rxBuf, this->m_rxLen);
        modfd(this->m_epollfd, this->m_sockfd, EPOLLIN, true, 0);
    }
}

static void Send(struct Tcp *this)
{
    if(this->m_ws == WS_NULL)
    {
        int headLen = strlen(this->m_txHead);
        int bodyLen = strlen(this->m_txBuf);
        send(this->m_sockfd, this->m_txHead, headLen, 0);
        send(this->m_sockfd, this->m_txBuf, bodyLen, 0);
        modfd(this->m_epollfd, this->m_sockfd, EPOLLIN, true, 0);
        LOG_INFO("====================================================================================================");
    }
    else
    {
        int txLen = strlen(this->m_txBuf);
        send(this->m_sockfd, this->m_txBuf, txLen, 0);
        modfd(this->m_epollfd, this->m_sockfd, EPOLLIN, true, 0);
        LOG_INFO("====================================================================================================");
    }
}

static void Recv(struct Tcp *this)
{
    this->m_rxLen = recv(this->m_sockfd, this->m_rxBuf, BUF_LEN, 0);
    Process(this);
}

static void Config(struct Tcp *this, int epollfd, int sockfd, struct sockaddr_in *addr, int expire)
{
    this->m_sockfd = sockfd;
    this->m_epollfd = epollfd;
    memcpy(&this->m_address, addr, sizeof(struct sockaddr_in));
    this->m_expire = expire;
}

Tcp* TcpInit(void)
{
    Tcp *this = malloc(sizeof(Tcp));
    memset(this, 0, sizeof(Tcp));

    this->Recv = Recv;
    this->Process = Process;
    this->Send = Send;
    this->Config = Config;
    return this;
}

void TcpRelease(Tcp* this)
{
    FREE(this);
}
