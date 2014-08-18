
/*
 * Copyright (C) nie950@gmail.com
 */


#ifndef __RBTREE_H__INCLUDED__
#define __RBTREE_H__INCLUDED__

typedef uint64_t rbtree_key_t;

typedef struct rbnode_s rbnode_t;
typedef struct rbtree_s rbtree_t;

typedef int (*rbnode_cmp_pt)(rbnode_t *,rbnode_t *);

#define  RBT_RED                0
#define  RBT_BLACK              1

#define rbt_init(T,p)               \
    do {                            \
        (T)->root = &((T)->nil);    \
        (T)->nil.c = RBT_BLACK;     \
        (T)->cmp = (p);             \
    }while(0)

struct rbnode_s {
    rbtree_key_t    k;        /*key*/
    char            c;        /*color*/
    rbnode_t       *p;        /*parent*/
    rbnode_t       *l;        /*left*/
    rbnode_t       *r;        /*right*/
};

struct rbtree_s {
    rbnode_t        *root;
    rbnode_t        nil;
    rbnode_cmp_pt   cmp;
};

rbnode_t* rbt_parent(rbtree_t *T,rbnode_t *nd);
rbnode_t* rbt_min(rbtree_t *T,rbnode_t *x);
rbnode_t* rbt_max(rbtree_t *T,rbnode_t * x);
rbnode_t* rbt_successor(rbtree_t *T,rbnode_t *x);
rbnode_t* rbt_predecessor(rbtree_t *T,rbnode_t *x);

int rbt_dept(rbtree_t * T);
int rbt_insert(rbtree_t *T,rbnode_t *z,int ingore);
int rbt_remove(rbtree_t *T,rbnode_t *z);

void rbt_inorder(rbtree_t * T,void (*visit)(rbnode_t *));

#endif