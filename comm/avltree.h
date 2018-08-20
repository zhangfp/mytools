#ifndef _AVL_TREE_H_
#define _AVL_TREE_H_

#include <string.h>
#include "rr.h"

#define KEY_LEN  RR_PID_LEN+RR_CID_LEN

typedef char KType[KEY_LEN];

//typedef struct node_info NodeInfo;

typedef struct node_info
{	
	int  node_num;
	char NodeIdList[32][RR_POPID_LEN];
} NodeInfo;

typedef struct AVLTreeNode{
    KType     key;                    // 关键字(键值)
    NodeInfo  value;
    int       height;
    struct AVLTreeNode *left;    // 左孩子
    struct AVLTreeNode *right;    // 右孩子
} AVLNode,*AVLTree;

// 获取AVL树的高度
int avltree_height(AVLTree tree);

// 前序遍历"AVL树"
void preorder_avltree(AVLTree tree);
// 中序遍历"AVL树"
void inorder_avltree(AVLTree tree);
// 后序遍历"AVL树"
void postorder_avltree(AVLTree tree);

void print_avltree(AVLTree tree, KType key, int direction);

// (递归实现)查找"AVL树x"中键值为key的节点
AVLNode* avltree_search(AVLTree x, KType key);
// (非递归实现)查找"AVL树x"中键值为key的节点
AVLNode* iterative_avltree_search(AVLTree x, KType key);

// 查找最小结点：返回tree为根结点的AVL树的最小结点。
AVLNode* avltree_minimum(AVLTree tree);
// 查找最大结点：返回tree为根结点的AVL树的最大结点。
AVLNode* avltree_maximum(AVLTree tree);

// 将结点插入到AVL树中，返回根节点
AVLNode* avltree_insert(AVLTree tree, KType key, NodeInfo *pvalue);

// 删除结点(key是节点值)，返回根节点
AVLNode* avltree_delete(AVLTree tree, KType key);

// 销毁AVL树
void destroy_avltree(AVLTree tree);


#endif