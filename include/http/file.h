#ifndef FILE_H
#define FILE_H

#include "http.h"

typedef struct FileType {
    char *key;
    char *value;
} FileType;

extern const Service FileService[];

#endif
