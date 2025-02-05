#ifndef MAP_H
#define MAP_H
#include <string.h>
#include <stdlib.h>

typedef struct KVNode {
    char *key;
    char *value;
    struct KVNode* next;
} KVNode;

typedef struct KVList {
    KVNode *m_head;
    KVNode *m_end;
    int m_size;

    void (*Insert)(struct KVList *this, char *key, char *value);
    char* (*Search)(struct KVList *this, char *key);
    void (*Delete)(struct KVList *this, char *key);
} KVList;

KVList* KVListInit(void);
void KVListRelease(KVList* this);

#endif 