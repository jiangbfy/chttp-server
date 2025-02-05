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

static void Send(struct Tcp *this)
{
    send(this->m_sockfd, this->m_http->m_respHead, this->m_http->m_respHeadLen, 0);
    if (this->m_http->m_respBody != NULL)
    {
        int sendLen = 0;
        while (sendLen < this->m_http->m_respBodyLen)
        {
            int len = send(this->m_sockfd, &this->m_http->m_respBody[sendLen], this->m_http->m_respBodyLen - sendLen, 0);
            if (len > 0) sendLen += len;
            else usleep(50000);
        }
    }
    modfd(this->m_epollfd, this->m_sockfd, EPOLLIN, true, 0);
    HttpRelease(this->m_http);
}

static void Recv(struct Tcp *this)
{
    this->m_http = HttpInit();
    this->m_http->Config(this->m_http, this->m_sockfd, &this->m_address);
    this->m_http->Recv(this->m_http);
    modfd(this->m_epollfd, this->m_sockfd, EPOLLOUT, true, 0);
}

static void Config(struct Tcp *this, int epollfd, int sockfd, struct sockaddr_in *addr)
{
    this->m_sockfd = sockfd;
    this->m_epollfd = epollfd;
    memcpy(&this->m_address, addr, sizeof(struct sockaddr_in));
}

Tcp* TcpInit(void)
{
    Tcp *this = malloc(sizeof(Tcp));
    memset(this, 0, sizeof(Tcp));

    this->Recv = Recv;
    this->Send = Send;
    this->Config = Config;
    return this;
}

void TcpRelease(Tcp* this)
{
    FREE(this);
}
