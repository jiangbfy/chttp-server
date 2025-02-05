#include "map.h"
#include "def.h"

static void Insert(struct KVList *this, char *key, char *value)
{
    KVNode *node = malloc(sizeof(KVNode));
    memset(node, 0, sizeof(KVNode));
    node->key = malloc(strlen(key) + 1);
    strcpy(node->key, key);
    node->value = malloc(strlen(value) + 1);
    strcpy(node->value, value);

    if (this->m_size == 0)
    {
        this->m_head = node;
        this->m_end = node;
    }
    else
    {
        this->m_end->next = node;
        this->m_end = node;
    }
    this->m_size++;
}

static char* Search(struct KVList *this, char *key)
{
    KVNode *temp = this->m_head;
    while (temp != NULL)
    {
        if (strcmp(temp->key, key) == 0)
        {
            return temp->value;
        }
        temp = temp->next;
    }
    return NULL;
}

void Delete(struct KVList *this, char *key)
{
    KVNode *prev = NULL;
    KVNode *temp = this->m_head;
    if (strcmp(temp->key, key) == 0)
    {
        this->m_size--;
        if(this->m_size == 0)
        {
            this->m_head = NULL;
            this->m_end = NULL;
        }
        else
        {
            this->m_head->next = temp->next;
        }
        FREE(temp->key);
        FREE(temp->value);
        FREE(temp);
        return;
    }
    prev = temp;
    temp = temp->next;
    while (temp != NULL)
    {
        if (strcmp(temp->key, key) == 0)
        {
            this->m_size--;
            prev->next = temp->next;
            if(this->m_end == temp)
            {
                this->m_end = prev;
            }
            FREE(temp->key);
            FREE(temp->value);
            FREE(temp);
            return;
        }
        prev = temp;
        temp = temp->next;
    }
}

KVList* KVListInit(void)
{
    KVList *this = malloc(sizeof(KVList));
    memset(this, 0, sizeof(KVList));

    this->Insert = Insert;
    this->Search = Search;
    this->Delete = Delete;
    return this;
}

void KVListRelease(KVList* this)
{
    KVNode *temp = this->m_head;
    KVNode *next = NULL;
    while (temp != NULL)
    {
        next = temp->next;
        FREE(temp->key);
        FREE(temp->value);
        FREE(temp);
        temp = next;
    }
    this->m_size = 0;
    FREE(this);
}