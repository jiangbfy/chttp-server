#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "def.h"
#include "tcp.h"

// 二叉树最大节点数
#define TREE_MAX 65536

typedef struct TreeNode
{
    // TCP套接字
    int id;
    // 节点深度
    int height;
    // 用户数据，指向tcp控制结构体
    void *data;
    // 二叉树左子树
    struct TreeNode *left;
    // 二叉树右子树
    struct TreeNode *right;
} TreeNode;

typedef struct Tree
{
    // 二叉树根节点
    TreeNode *m_root;
    // 二叉树节点数量
    int m_size;

    // 插入节点
    void (*Insert)(struct Tree *this, int id, void *data);
    // 删除节点
    void (*Delete)(struct Tree *this, int id);
    // 根据id搜索节点
    TreeNode* (*Search)(struct Tree *this, int id);
    // 遍历节点
    void (*ForEach)(struct Tree *this);
} Tree;

Tree* TreeInit(void);
void TreeRelease(Tree* this);

#endif