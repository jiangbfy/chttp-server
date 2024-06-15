#include "http.h"
#include "controller.h"
#include "log.h"

static char* GetLine(char *line, char *end)
{
    char *pbuf = strstr(line, "\r\n");
    if(pbuf == NULL)
        return NULL;
    pbuf[0] = 0;
    pbuf[1] = 0;
    return pbuf;
}
//解析http请求
static void ParseReq(struct Http *this, char *head, char *end)
{
    char *pbuf = head;

    if(memcmp(pbuf, "GET", 3) == 0)
    {
        this->m_method = METHOD_GET;
        LOG_INFO("METHOD: GET");
        pbuf += 4;
    }
    else if(memcmp(pbuf, "POST", 4) == 0)
    {
        this->m_method = METHOD_POST;
        LOG_INFO("METHOD: POST");
        pbuf += 5;
    }
    else if(memcmp(pbuf, "OPTION", 6) == 0)
    {
        LOG_INFO("METHOD: OPTION");
        this->m_method = METHOD_OPTION;
        this->m_line_state = LINE_ERROR;
        pbuf += 7;
        this->m_url = pbuf;
    }
    else
    {
        this->m_line_state = LINE_ERROR;
        pbuf = strstr(pbuf, " ");
        pbuf++;
        this->m_url = pbuf;
        return;
    }

    this->m_url = pbuf;
    pbuf = strstr(this->m_url, " ");
    pbuf++;

    if(memcmp(pbuf, "HTTP/1.1", 8) != 0)
    {
        this->m_line_state = LINE_ERROR;
    }
}
//解析http头
static void ProcessRequest(struct Http *this, char *request, int reqLen)
{
    request[reqLen] = 0;
    char *msgEnd = request + reqLen;
    char *reqEnd = GetLine(request, msgEnd);
    if(reqEnd == NULL)
    {
        this->m_line_state = LINE_ERROR;
        return;
    }
    ParseReq(this, request, reqEnd);
    char *head = reqEnd + 2;
    char *headEnd = NULL;
    int headLen = 1;
    while(headLen > 0)
    {
        headEnd = GetLine(head, msgEnd);
        headLen = headEnd - head;
        if(headEnd == NULL)
        {
            this->m_line_state = LINE_ERROR;
            return;
        }
        head = headEnd + 2;
    }
    this->m_body = head;
    if(this->m_line_state == LINE_OK) this->m_line_state = LINE_FIN;
    LOG_INFO("URL: %s", this->m_url);
    int bodyLen = reqLen - (this->m_body - request);
    if(bodyLen < LOG_LEN) LOG_INFO("BODY: %s", this->m_body);
}

static void ProcessResponse(struct Http *this, char *response)
{
    char *pbuf = response;
    int len = 0;

    if(this->m_line_state != LINE_FIN)
    {
        len = -1;
        response[RES_BODY] = 0;
    }
    else
    {
        len = ControllerParse(&response[RES_BODY], this);
        if(len < LOG_LEN) LOG_INFO("Response: %s", &response[RES_BODY]);
    }

    if(len < 0)
    {
        len = 0;
        if(this->m_method == METHOD_OPTION)
        {
            //OPTION方法处理
            strcpy(pbuf, "HTTP/1.1 200\r\n");
            pbuf += 14;
        }
        else
        {
            //请求错误
            strcpy(pbuf, "HTTP/1.1 400 Bad Request\r\n");
            pbuf += 26;
        }
    }
    else
    {
        strcpy(pbuf, "HTTP/1.1 200 OK\r\n");
        pbuf += 17;
    }
    //允许跨域
    strcpy(pbuf, "Access-Control-Allow-Credentials: true\r\n");
    pbuf += 40;
    strcpy(pbuf, "Access-Control-Allow-Origin: *\r\n");
    pbuf += 32;
    strcpy(pbuf, "Access-Control-Allow-Methods: GET, POST\r\n");
    pbuf += 41;
    strcpy(pbuf, "Access-Control-Allow-Headers: Access-Control-Allow-Headers, Content-Length, Accept, Origin, Host, Connection, Keep-Alive, Content-Type\r\n");
    pbuf += 136;
    strcpy(pbuf, "Connection: keep-alive\r\n");
    pbuf += 24;
    strcpy(pbuf, "Keep-Alive: timeout=60\r\n");
    pbuf += 24;
    strcpy(pbuf, "Content-type: application/json\r\n");
    pbuf += 32;
    sprintf(pbuf, "Content-length: %d\r\n\r\n", len);
}
//get参数装json
int ParseUrl(char *url, cJSON *object)
{
    char *paraEnd = strstr(url, " ");
    *paraEnd = 0;
    char *para = strstr(url, "?");
    if(para == NULL)
    {
        return -1;
    }

    char *key = NULL, *value = NULL;
    while(true)
    {
        key = para + 1;
        value = strstr(key, "=");
        *value = 0;
        value += 1;
        para = strstr(value, "&");
        if(para == NULL)
        {
            cJSON_AddStringToObject(object, key, value);
            break;
        }
        else
        {
            *para = 0;
            cJSON_AddStringToObject(object, key, value);
        }
    }
    return 0;
}

Http* HttpInit(void)
{
    Http *this = malloc(sizeof(Http));
    memset(this, 0, sizeof(Http));

    this->ProcessRequest = ProcessRequest;
    this->ProcessResponse = ProcessResponse;
    return this;
}
void  HttpRelease(Http *this)
{
    FREE(this);
}