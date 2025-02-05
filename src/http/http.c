#include "http.h"
#include "log.h"
#include "cmd.h"
#include "user.h"
#include "file.h"

const static ServiceClass ServiceClassList[] = {
    {"/api/cmd", CmdService},
    {"/api/user", UserService},
    {"/api/file", FileService},
    {"NULL", NULL}
};

static void Config(struct Http *this, int sockfd, struct sockaddr_in *addr)
{
    this->m_sockfd = sockfd;
    memcpy(&this->m_address, addr, sizeof(struct sockaddr_in));
}

static void Parse(struct Http *this, char *line, int len)
{
    if (this->m_state == STATE_REQ)
    {
        char *req = line;
        char *reqEnd = strstr(req, " ");
        *reqEnd = 0;
        if (strcmp(req, "POST") == 0) this->m_method = METHOD_POST;
        else if (strcmp(req, "GET") == 0) this->m_method = METHOD_GET;
        else if (strcmp(req, "OPTIONS") == 0) this->m_method = METHOD_OPTIONS;
        else { this->m_method = METHOD_NULL; this->m_state = STATE_ERR; }
        char *url = reqEnd + 1;
        char *urlEnd = strstr(url, " ");
        *urlEnd = 0;
        int urlLen = urlEnd - url;
        this->m_url = malloc(urlLen + 1);
        strcpy(this->m_url, url);
        this->m_state = STATE_HEAD;
        LOG_INFO("%s %s", req, this->m_url);
    }
    else if(this->m_state == STATE_HEAD)
    {
        if (len == 0) { this->m_state = STATE_BODY; return; }
        char *key = line;
        char *keyEnd = strstr(key, ":");
        *keyEnd = 0;
        char *value = keyEnd + 2;
        this->m_header->Insert(this->m_header, key, value);
    }
}

void ParseParam(struct Http *this, char *param)
{
    int len = strlen(param);
    char *line = malloc(len + 1);
    strcpy(line, param);
    char *key = line;
    char *keyEnd = strstr(key, "=");
    if (keyEnd == NULL) return;
    *keyEnd = 0;
    char *value = keyEnd + 1;
    char *valueEnd = strstr(value, "&");
    if (valueEnd != NULL) *valueEnd = 0;
    while (keyEnd != NULL && valueEnd != NULL)
    {
        this->m_param->Insert(this->m_param, key, value);
        key = valueEnd + 1;
        keyEnd = strstr(key, "=");
        if (keyEnd == NULL) break;
        *keyEnd = 0;
        value = keyEnd + 1;
        valueEnd = strstr(value, "&");
        if (valueEnd == NULL) break;
        *valueEnd = 0;
    }
    if (keyEnd != NULL && valueEnd == NULL) this->m_param->Insert(this->m_param, key, value);
    FREE(line);
}

void AddOkHead(struct Http *this)
{
    this->m_respHead = malloc(RESP_HEAD_LEN);
    char *pbuf = this->m_respHead;

    strcpy(pbuf, "HTTP/1.1 200 OK \r\n");
    pbuf += 18;
    strcpy(pbuf, "Access-Control-Allow-Credentials: true\r\n");
    pbuf += 40;
    strcpy(pbuf, "Access-Control-Allow-Origin: *\r\n");
    pbuf += 32;
    strcpy(pbuf, "Access-Control-Allow-Methods: GET, POST\r\n");
    pbuf += 41;
    strcpy(pbuf, "Access-Control-Allow-Headers: Content-Length, Accept, Origin, Host, Connection, Keep-Alive, Content-Type\r\n");
    pbuf += 106;
    strcpy(pbuf, "Connection: keep-alive\r\n");
    pbuf += 24;
    strcpy(pbuf, "Keep-Alive: timeout=60\r\n");
    pbuf += 24;
    if (this->m_fileType != NULL)
    {
        int contentTypeLen = sprintf(pbuf, "Content-Type: %s\r\n", this->m_fileType);
        pbuf += contentTypeLen;
    }
    else
    {
        strcpy(pbuf, "Content-Type: application/json\r\n");
        pbuf += 32;
    }
    int contentLength = sprintf(pbuf, "Content-Length: %d\r\n\r\n", this->m_respBodyLen);
    this->m_respHeadLen = (pbuf - this->m_respHead) + contentLength;
}

void AddBadHead(struct Http *this)
{
    this->m_respHead = malloc(RESP_HEAD_LEN);
    char *pbuf = this->m_respHead;

    strcpy(pbuf, "HTTP/1.1 400 Bad Request \r\n");
    pbuf += 26;
    strcpy(pbuf, "Access-Control-Allow-Credentials: true\r\n");
    pbuf += 40;
    strcpy(pbuf, "Access-Control-Allow-Origin: *\r\n");
    pbuf += 32;
    strcpy(pbuf, "Access-Control-Allow-Methods: GET, POST\r\n");
    pbuf += 41;
    strcpy(pbuf, "Access-Control-Allow-Headers: Content-Length, Accept, Origin, Host, Connection, Keep-Alive, Content-Type\r\n");
    pbuf += 106;
    strcpy(pbuf, "Connection: keep-alive\r\n");
    pbuf += 24;
    strcpy(pbuf, "Keep-Alive: timeout=60\r\n");
    pbuf += 24;
    strcpy(pbuf, "Content-Length: 0\r\n\r\n");
    pbuf += 21;
    this->m_respHeadLen = (pbuf - this->m_respHead);
}

void AddOptionsHead(struct Http *this)
{
    this->m_respHead = malloc(RESP_HEAD_LEN);
    char *pbuf = this->m_respHead;

    strcpy(pbuf, "HTTP/1.1 200 \r\n");
    pbuf += 14;
    strcpy(pbuf, "Access-Control-Allow-Credentials: true\r\n");
    pbuf += 40;
    strcpy(pbuf, "Access-Control-Allow-Origin: *\r\n");
    pbuf += 32;
    strcpy(pbuf, "Access-Control-Allow-Methods: GET, POST\r\n");
    pbuf += 41;
    strcpy(pbuf, "Access-Control-Allow-Headers: Content-Length, Accept, Origin, Host, Connection, Keep-Alive, Content-Type\r\n");
    pbuf += 106;
    strcpy(pbuf, "Connection: keep-alive\r\n");
    pbuf += 24;
    strcpy(pbuf, "Keep-Alive: timeout=60\r\n");
    pbuf += 24;
    strcpy(pbuf, "Content-Length: 0\r\n\r\n");
    pbuf += 21;
    this->m_respHeadLen = (pbuf - this->m_respHead);
}

static void Process(struct Http *this)
{
    if (this->m_method == METHOD_OPTIONS) goto HttpOptions;

    int firstLen = 0;
    ServiceClass *servClass = (ServiceClass *)ServiceClassList;
    Service *serv = NULL;
    while (servClass->m_service != NULL)
    {
        firstLen = strlen(servClass->m_first);
        if (memcmp(servClass->m_first, this->m_url, firstLen) == 0) break;
        servClass++;
    }
    if (servClass->m_service == NULL) { this->function = NULL; goto HttpBad; }
    
    serv = (Service *)servClass->m_service;
    char *secondUrl = &this->m_url[firstLen];
    int secondLen = 0;
    char *param = strstr(this->m_url, "?");
    if (param != NULL) {
        param = param + 1;
        ParseParam(this, param);
    }
    while (serv->function != NULL)
    {
        secondLen = strlen(serv->m_second);
        if(memcmp(serv->m_second, secondUrl, secondLen)  == 0) break;
        serv++;
    }
    if(serv->function == NULL) { this->function = NULL; goto HttpBad; }

    if (this->m_method != serv->m_method) goto HttpBad;

    this->function = serv->function;
    this->function(this);
    AddOkHead(this);
    return;

HttpBad:
    AddBadHead(this);
    return;
HttpOptions:
    AddOptionsHead(this);
    return;
}

static void Recv(struct Http *this)
{
    int noDataCnt = 0, lineLen = 0, hasLen = 0, dealLen = 0, rxLen = 0, bodyLen = 0;
    char *lineBeg = NULL, *lineEnd = NULL;
    char *rxBuf = malloc(BUF_LEN);
    char *parseBuf = malloc(BUF_LEN);

    while (true)
    {
        rxLen = recv(this->m_sockfd, &rxBuf[hasLen], RECV_LEN - hasLen, 0);
        if (rxLen > 0)
        {
            noDataCnt = 0;
            dealLen = 0;
            hasLen += rxLen;
            rxBuf[hasLen] = 0;
            if (this->m_state < STATE_BODY)
            {
                lineBeg = rxBuf;
                lineEnd = strstr(lineBeg, "\r\n");
                while (lineEnd != NULL)
                {
                    lineLen = lineEnd - lineBeg;
                    memcpy(parseBuf, lineBeg, lineLen);
                    parseBuf[lineLen] = 0;
                    Parse(this, parseBuf, lineLen);
                    lineBeg = lineEnd + 2;
                    dealLen += (lineLen + 2);
                    hasLen -= (lineLen + 2);
                    if (this->m_state == STATE_BODY) break;
                    lineEnd = strstr(lineBeg, "\r\n");
                }
                if (hasLen > 0)
                {
                    memcpy(rxBuf, &rxBuf[dealLen], hasLen);
                    rxBuf[hasLen] = 0;
                }
                if (this->m_state == STATE_BODY && this->m_method == METHOD_POST)
                {
                    char *length = this->m_header->Search(this->m_header, "Content-Length");
                    int lengthInt = atoi(length);
                    this->m_bodyLen = lengthInt;
                    this->m_body = malloc(lengthInt + 1);
                    if (hasLen > 0)
                    {
                        memcpy(this->m_body, rxBuf, hasLen);
                        bodyLen = hasLen;
                        hasLen = 0;
                        this->m_body[bodyLen] = 0;
                    }
                }
            }
            else
            {
                if (this->m_method == METHOD_POST)
                {
                    memcpy(&this->m_body[bodyLen], rxBuf, hasLen);
                    bodyLen += hasLen;
                    this->m_body[bodyLen] = 0;
                }
                hasLen = 0;
            }
        }
        else
        {
            noDataCnt++;
            usleep(50000);
        }
        if (noDataCnt > 1) break;
    }
    FREE(rxBuf);
    FREE(parseBuf);

    Process(this);
}

Http* HttpInit(void)
{
    Http *this = malloc(sizeof(Http));
    memset(this, 0, sizeof(Http));

    this->Config = Config;
    this->Recv = Recv;
    this->m_header = KVListInit();
    this->m_param = KVListInit();
    return this;
}
void  HttpRelease(Http *this)
{
    KVListRelease(this->m_header);
    KVListRelease(this->m_param);
    if (this->m_url != NULL) FREE(this->m_url);
    if (this->m_body != NULL) FREE(this->m_body);
    if (this->m_respHead != NULL) FREE(this->m_respHead);
    if (this->m_respBody != NULL) FREE(this->m_respBody);
    if (this->m_fileType != NULL) FREE(this->m_fileType);
    FREE(this);
}