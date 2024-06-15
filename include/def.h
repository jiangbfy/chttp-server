#ifndef DEF_H
#define DEF_H

//布尔类型定义
typedef enum {false = 0, true} bool;
#define NULL ((void *)0)
#define MAX(a, b) ((a >= b) ? a : b)
//变量名转字符串
#define NAME_TO_STR(name) (#name)
//断言
#define ASSERT(cond) \
do { \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #cond, __FILE__, __LINE__); \
        abort(); \
    } \
} while (0)
//释放内存
#define FREE(cond) \
do { \
    free(cond); \
    cond = NULL; \
} while (0)

#endif