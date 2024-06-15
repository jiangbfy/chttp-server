#include "tree.h"
#include "log.h"

static TreeNode* NewNode(int id, void *data)
{
    TreeNode *node = malloc(sizeof(TreeNode));
    node->id = id;
    node->data = data;
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    return node;
}

static int Height(TreeNode* node)
{
    if(node == NULL) return 0;
    return node->height;
}

static int Balance(TreeNode* node)
{
    if(node == NULL) return 0;
    return (Height(node->left) - Height(node->right));
}

TreeNode* RightRotate(TreeNode* y) {
    TreeNode* x = y->left;
    TreeNode* z = x->right;
    x->right = y;
    y->left = z;
    y->height = MAX(Height(y->left), Height(y->right)) + 1;
    x->height = MAX(Height(x->left), Height(x->right)) + 1;
    return x;
}

TreeNode* LeftRotate(TreeNode* x) {
    TreeNode* y = x->right;
    TreeNode* z = y->left;
    y->left = x;
    x->right = z;
    x->height = MAX(Height(x->left), Height(x->right)) + 1;
    y->height = MAX(Height(y->left), Height(y->right)) + 1;
    return y;
}

static TreeNode* InsertNode(struct Tree *this, TreeNode* node, int id, void *data)
{
    if(node == NULL)
    {
        this->m_size++;
        return NewNode(id, data);
    }

    if (id < node->id)
        node->left = InsertNode(this, node->left, id, data);
    else if (id > node->id)
        node->right = InsertNode(this, node->right, id, data);
    else
        return node;

    node->height = 1 + MAX(Height(node->left), Height(node->right));

    int balance = Balance(node);
    //平衡二叉树高度
    if (balance > 1 && id < node->left->id)
        return RightRotate(node);
    if (balance < -1 && id > node->right->id)
        return LeftRotate(node);
    if (balance > 1 && id > node->left->id) {
        node->left = LeftRotate(node->left);
        return RightRotate(node);
    }
    if (balance < -1 && id < node->right->id) {
        node->right = RightRotate(node->right);
        return LeftRotate(node);
    }

    return node;
}
static TreeNode* MinValueNode(TreeNode* node) {
    TreeNode* current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

static TreeNode* DeleteNode(struct Tree *this, TreeNode* node, int id) {
    if (node == NULL)
        return node;

    if (id < node->id)
        node->left = DeleteNode(this, node->left, id);
    else if (id > node->id)
        node->right = DeleteNode(this, node->right, id);
    else {
        if ((node->left == NULL) || (node->right == NULL)) {
            TreeNode* temp = node->left ? node->left : node->right;
            //若没有子节点直接删除
            if (temp == NULL) {
                temp = node;
                if(temp->data != NULL) {
                    FREE(temp->data);
                }
                FREE(temp);
                node = NULL;
            } else{
                //若有子节点，删除node节点，并将子节点移到当前位置
                if(node->data != NULL) {
                    FREE(node->data);
                }
                memcpy(node, temp, sizeof(TreeNode));
                FREE(temp);
            }
            this->m_size--;
        } else {
            //将temp和node的data对调，并删除temp节点
            TreeNode* temp = MinValueNode(node->right);
            node->id = temp->id;
            void *data = node->data;
            node->data = temp->data;
            temp->data = data;
            node->right = DeleteNode(this, node->right, temp->id);
        }
    }

    if (node == NULL)
        return node;

    node->height = 1 + MAX(Height(node->left), Height(node->right));

    int balance = Balance(node);
    //平衡二叉树高度
    if (balance > 1 && Balance(node->left) >= 0)
        return RightRotate(node);
    if (balance > 1 && Balance(node->left) < 0) {
        node->left = LeftRotate(node->left);
        return RightRotate(node);
    }
    if (balance < -1 && Balance(node->right) <= 0)
        return LeftRotate(node);
    if (balance < -1 && Balance(node->right) > 0) {
        node->right = RightRotate(node->right);
        return LeftRotate(node);
    }

    return node;
}

static TreeNode* SearchNode(TreeNode* node, int id) {
    if (node == NULL || node->id == id)
        return node;

    if (node->id < id)
        return SearchNode(node->right, id);

    return SearchNode(node->left, id);
}

static void ForEachNode(TreeNode* node) {
    if (node != NULL) {
        //中序遍历
        ForEachNode(node->left);
        printf("%d ", node->id);
        ForEachNode(node->right);
    }
}

static void Insert(struct Tree *this, int id, void *data)
{
    this->m_root = InsertNode(this, this->m_root, id, data);
}

static void Delete(struct Tree *this, int id)
{
    this->m_root = DeleteNode(this, this->m_root, id);
}

static TreeNode* Search(struct Tree *this, int id)
{
    return SearchNode(this->m_root, id);
}

static void ForEach(struct Tree *this)
{
    ForEachNode(this->m_root);
    printf("\n");
}

Tree* TreeInit(void)
{
    Tree *this = malloc(sizeof(Tree));
    memset(this, 0, sizeof(Tree));

    this->Insert = Insert;
    this->Delete = Delete;
    this->Search = Search;
    this->ForEach = ForEach;
    return this;
}
void TreeRelease(Tree* this)
{
    while(this->m_size > 0) {
        close(this->m_root->id);
        this->m_root = DeleteNode(this, this->m_root, this->m_root->id);
    }
    FREE(this);
}
