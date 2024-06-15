#include "tcp.h"

static void modfd(int epollfd, int fd, int ev, int one_shot, int trigMode)
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
    Http *http = HttpInit();
    http->m_address = &this->m_address;
    http->ProcessRequest(http, this->m_rxBuf, this->m_rxLen);
    http->ProcessResponse(http, this->m_txBuf);
    HttpRelease(http);
    modfd(this->m_epollfd, this->m_sockfd, EPOLLOUT, true, 0);
}

static void Send(struct Tcp *this)
{
    int headLen = strlen(this->m_txBuf);
    int bodyLen = strlen(&this->m_txBuf[RES_BODY]);
    send(this->m_sockfd, this->m_txBuf, headLen, 0);
    send(this->m_sockfd, &this->m_txBuf[RES_BODY], bodyLen, 0);
    modfd(this->m_epollfd, this->m_sockfd, EPOLLIN, true, 0);
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
