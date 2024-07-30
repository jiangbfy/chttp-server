#include <stdio.h>
#include <string.h>
#include "def.h"
#include "ws_service.h"
#include "log.h"
#include "base64_encoder.h"
#include "sha1.h"
#include "http.h"

int WsConnect(char *argOut, Http *this)
{
    int ret = 0;
    char *pbuf = argOut;
    char *key = strstr(this->m_head, "Sec-WebSocket-Key");
    if(key == NULL) return 0;
    key += 19;
    char *keyEnd = strstr(key, "\r\n");
    if(key == NULL) return 0;
    char recvKey[64];
    int recvKeyLen = keyEnd - key;
    memcpy(recvKey, key, recvKeyLen);
    strcpy(&recvKey[recvKeyLen], "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    LOG_INFO("RecvKey: %s", recvKey);
    int sha1_size = 0;
    int base64_len = 0;
    char* sha1Str = crypt_sha1((uint8_t*)recvKey, strlen(recvKey), &sha1_size);
    char* base64Str = base64_encode((uint8_t*)sha1Str, sha1_size, &base64_len);
    LOG_INFO("AcceptKey: %s", base64Str);
    strcpy(pbuf, "HTTP/1.1 101 Switching Protocols\r\n");
    pbuf += 34;
    strcpy(pbuf, "Upgrade: websocket\r\n");
    pbuf += 20;
    strcpy(pbuf, "Connection: upgrade\r\n");
    pbuf += 21;
    ret = sprintf(pbuf, "Sec-WebSocket-Accept: %s\r\n\r\n", base64Str);
    this->m_ws = WS_NORMAL;
    ret += (pbuf - argOut);
    return ret;
}

int WsPublish(char *argOut, Http *this)
{
    this->m_ws = WS_PUBLISH;
    int ret = sprintf(argOut, "{\"ws\":\"success\"}");
    return ret;
}

const Service WsServiceList[] = {
    {"/ws", WsConnect},
    {"/publish", WsPublish},
    {"NULL", NULL}
};