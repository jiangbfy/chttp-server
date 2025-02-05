#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "cJSON.h"
#include "log.h"
#include "common.h"
#include "syspara.h"

const static FileType FileTypeList[] = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript" },
    { NULL,     NULL }
};

void Upload(Http *http)
{
    char *fileEnd = searchStrNeg(&http->m_body[http->m_bodyLen - 3], "\r\n", 512);
    char *fileStart = searchStrPos(http->m_body, "\r\n\r\n", 512);
    fileStart += 4;
    int fileLen = fileEnd - fileStart;
    char *fileName = searchStrPos(http->m_body, "filename=", 512);
    fileName += 10;
    char *fileNameEnd = searchStrPos(fileName, "\"", 512);
    int fileNameLen = fileNameEnd - fileName;
    char *path = malloc(512);
    int pathLen = strlen(syspara.workPath);
    memcpy(path, syspara.workPath, pathLen);
    memcpy(&path[pathLen], "/static/", 8);
    pathLen += 8;
    memcpy(&path[pathLen], fileName, fileNameLen);
    pathLen += fileNameLen;
    path[pathLen] = 0;

    FILE *fp = fopen(path, "wb");
    fwrite(fileStart, sizeof(char), fileLen, fp);
    fflush(fp);
    fclose(fp);

    http->m_respBody = malloc(512);
    char *url = strstr(path, "/static");
    http->m_respBodyLen =  sprintf(http->m_respBody, "{\"code\": 200, \"url\": %s}", url);
    FREE(path);
}

void Download(Http *http)
{
    char *url = strstr(http->m_url, "/static");
    if (url == NULL) return;
    char *path = malloc(512);
    int pathLen = strlen(syspara.workPath);
    memcpy(path, syspara.workPath, pathLen);
    int len = sprintf(&path[pathLen], "%s", url);
    pathLen += len;
    char *key = searchStrNeg(&path[pathLen - 1], ".", 10);
    if (key != NULL)
    {
        FileType *temp = (FileType *)FileTypeList;
        while (temp->key != NULL)
        {
            if (strcmp(temp->key, key) == 0) break;
            temp++;
        }
        if (temp->key != NULL)
        {
            len = strlen(temp->value);
            http->m_fileType = malloc(len + 1);
            strcpy(http->m_fileType, temp->value);
        }
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) return;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    http->m_respBody = malloc(size + 1);
    int readLen = fread(http->m_respBody, sizeof(char), size, fp);
    if (readLen > 0) http->m_respBodyLen = readLen;
    fclose(fp);
    FREE(path);
}

const Service FileService[] = {
    {"/upload", METHOD_POST, Upload},
    {"/download", METHOD_GET, Download},
    {"NULL", METHOD_NULL, NULL}
};
