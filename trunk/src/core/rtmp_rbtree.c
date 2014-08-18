
/*
 * Copyright (C) nie950@gmail.com
 */


/*
 * The red-black tree code is based on the algorithm described in
 * the "Introduction to Algorithms" by Cormen, Leiserson and Rivest.
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

rbnode_t * rbt_max(rbtree_t *T,rbnode_t * x) 
{
    while ((x != &T->nil) && (x->r != &T->nil)) {
        x = x->r;
    }
    if (x == &T->nil) {
        x = 0;
    }
    return x;
}

rbnode_t * rbt_min(rbtree_t *T,rbnode_t *x) 
{
    while ((x != &T->nil) && (x->l != &T->nil)) {
        x = x->l;
    }
    if (x == &T->nil) {
        x = 0;
    }
    return x;
}

rbnode_t * rbt_successor(rbtree_t *T,rbnode_t *x)
{
    rbnode_t *y;
    if (x->r != &T->nil) {
        y = rbt_min(T,x->r);
    } else {
        y = x->p;
        while ((y != &T->nil) && (y->r == x)) {
            x = y;
            y = x->p;
        }
        if (y == &T->nil) {
            y = 0;
        }
    }
    return y;
}

rbnode_t * rbt_predecessor(rbtree_t *T,rbnode_t *x)
{
    rbnode_t *y;
    if (x->l != &T->nil) {
        y = rbt_max(T,x->l);
    } else {
        y = x->p;
        while ((y != &T->nil) && (y->l == x)) {
            x = y;
            y = x->p;
        }
        if (y == &T->nil) {
            y = 0;
        }
    }
    return y;
}

void rbt_inorder(rbtree_t * T,void (*visit)(rbnode_t *))
{
    rbnode_t *x;
    x = T->root;
    if (x != &T->nil) {
        x = rbt_min(T,x);
        while (x != 0) {
            visit(x);
            x = rbt_successor(T,x);
        }
    }
    return ;
}

void _rbt_left_rotate(rbtree_t *T,rbnode_t *x)
{
    rbnode_t * y;
    y = x->r;
    y->p = x->p;
    if (x->p == &T->nil) {
        T->root = y;
    } else {
        if (x->p->l == x) {
            x->p->l = y;
        } else {
            x->p->r = y;
        }
    }
    x->r = y->l;
    if (x->r != &T->nil) {
        x->r->p = x;
    }
    x->p = y;
    y->l = x;
}

void _rbt_right_rotate(rbtree_t *T,rbnode_t *x)
{
    rbnode_t *y;
    y = x->l;
    y->p = x->p;
    if (x->p == &T->nil) {
        T->root = y;
    } else {
        if (x->p->l == x) {
            x->p->l = y;
        } else {
            x->p->r = y;
        }
    }
    x->l = y->r;
    if (x->l != &T->nil) {
        x->l->p = x;
    }
    x->p = y;
    y->r = x;
}

void _rbt_insert_fixup(rbtree_t *T,rbnode_t *z) 
{
    while (z->p->c == RBT_RED) {
        if (z->p == z->p->p->l) {
            if (z->p->p->r->c == RBT_RED) {
                z->p->p->r->c = RBT_BLACK;      /*case 3*/
                z->p->c = RBT_BLACK;            /*case 3*/
                z->p->p->c = RBT_RED;           /*case 3*/
                z = z->p->p;                    /*case 3*/
            } else {
                if (z == z->p->r) {
                    z = z->p;                   /*case 2*/
                    _rbt_left_rotate(T,z);      /*case 2*/
                }
                z->p->c = RBT_BLACK;            /*case 1*/
                z->p->p->c = RBT_RED;           /*case 1*/
                _rbt_right_rotate(T,z->p->p);   /*case 1*/
            }
        } else {
            if (z->p->p->l->c == RBT_RED) {
                z->p->p->l->c = RBT_BLACK;      /*case 3*/
                z->p->c = RBT_BLACK;            /*case 3*/
                z->p->p->c = RBT_RED;           /*case 3*/
                z = z->p->p;                    /*case 3*/
            } else {
                if (z == z->p->l) {
                    z = z->p;                   /*case 2*/
                    _rbt_right_rotate(T,z);     /*case 2*/
                }
                z->p->c = RBT_BLACK;            /*case 1*/
                z->p->p->c = RBT_RED;           /*case 1*/
                _rbt_left_rotate(T,z->p->p);    /*case 1*/
            }
        }
    }
    T->root->c = RBT_BLACK;
    return ;
}

int rbt_insert(rbtree_t *T,rbnode_t *z,int ignore)
{
    rbnode_t *x,*y;
    int rc;

    x = T->root;
    y = x;
    while (x != &T->nil) {
        y = x;

        rc = T->cmp(z,x);
        if ((rc == 0) && (!ignore)) {
            return 1;
        }

        if (rc <= 0) {
            x = x->l;
        } else {
            x = x->r;
        }
    }
    z->l = z->r = z->p = &T->nil;
    if (y == &T->nil) {
        T->root = z;
    } else {

        rc = T->cmp(z,y);
        if (rc <= 0) {
            y->l = z;
        } else {
            y->r = z;
        }
        z->p = y;
    }
    z->c = RBT_RED;

    _rbt_insert_fixup(T,z);
    return 0;
}

void _rbt_transplant(rbtree_t *T,rbnode_t *u,rbnode_t *v) 
{
    if (u->p == &T->nil) {
        T->root = v;
    } else {
        if (u->p->l == u) {
            u->p->l = v;
        } else {
            u->p->r = v;
        }    
    }
    if (v != &T->nil) {
        v->p = u->p;
    }
    return ;
}

int _rbt_remove_fixup(rbtree_t *T,rbnode_t *x)
{
    rbnode_t * w;
    while ((x != T->root) && (x->c == RBT_BLACK)) {
        if (x == x->p->l) {
            w = x->p->r;
            if (w->c == RBT_RED) {
                x->p->c = RBT_RED;
                w->c = RBT_BLACK;
                _rbt_left_rotate(T,x->p);         /*case 4*/
                w = x->p->r;                      /*case 4*/
            }
            if (w->c == RBT_BLACK) {
                if ((w->l->c == RBT_BLACK) && (w->r->c == RBT_BLACK)) {
                    w->c = RBT_RED;                /*case 3*/
                    x = x->p;                      /*case 3*/
                } else {
                    if (w->r->c == RBT_BLACK) {    /*case 2*/
                        w->c = RBT_RED;            /*case 2*/
                        w->l->c = RBT_BLACK;       /*case 2*/
                        _rbt_right_rotate(T,w);    /*case 2*/
                        w = x->p->r;               /*case 2*/
                    }
                    w->c = w->p->c;                /*case 1*/
                    w->p->c = RBT_BLACK;           /*case 1*/
                    w->r->c = RBT_BLACK;           /*case 1*/
                    _rbt_left_rotate(T,x->p);      /*case 1*/
                    x = T->root;
                }
            }
        } else {
            w = x->p->l;
            if (w->c == RBT_RED) {
                x->p->c = RBT_RED;
                w->c = RBT_BLACK;
                _rbt_right_rotate(T,x->p);        /*case 4*/
                w = x->p->l;                      /*case 4*/
            }
            if (w->c == RBT_BLACK) {
                if ((w->l->c == RBT_BLACK) && (w->r->c == RBT_BLACK)) {
                    w->c = RBT_RED;                /*case 3*/
                    x = x->p;                      /*case 3*/
                } else {
                    if (w->l->c == RBT_BLACK) {    /*case 2*/
                        w->c = RBT_RED;            /*case 2*/
                        w->r->c = RBT_BLACK;       /*case 2*/
                        _rbt_left_rotate(T,w);     /*case 2*/
                        w = x->p->l;               /*case 2*/
                    }
                    w->c = w->p->c;                /*case 1*/
                    w->p->c = RBT_BLACK;           /*case 1*/
                    w->l->c = RBT_BLACK;           /*case 1*/
                    _rbt_right_rotate(T,x->p);     /*case 1*/
                    x = T->root;
                }
            }
        }
    }
    x->c = RBT_BLACK;
    return 0;
}

int rbt_remove(rbtree_t *T,rbnode_t *z)
{
    int c;
    rbnode_t *x;
    do {
        rbnode_t * y;
        c = z->c;
        if (z->r == &T->nil) {
            x = z->l;
            _rbt_transplant(T,z,z->l);
            x->p = z->p;        /*if x is nil*/
            break;
        }
        y = rbt_min(T,z->r);
        c = y->c;
        y->c = z->c;
        x = y->r;
        if (y != z->r) {
            _rbt_transplant(T,y,y->r);
            x->p = y->p;        /*if x is nil*/
            y->r = z->r;
            y->r->p = y;
        } else {
            x->p = y;           /*if x is nil*/
        }
        _rbt_transplant(T,z,y);
        y->l = z->l;
        if (z->l != &T->nil) {
            z->l->p = y;
        }
    } while (0);

    if ((c == RBT_BLACK )&&(x->c == RBT_BLACK)) {
        _rbt_remove_fixup(T,x);
    } else {
        x->c = RBT_BLACK;
    }

    z->p = z->l = z->r = NULL;

    return 0;
}

rbnode_t* rbt_parent(rbtree_t *T,rbnode_t *nd)
{
    if (nd->p != &T->nil) {
        return nd->p;
    }
    return 0;
}

int _rbt_dept(rbtree_t * T,rbnode_t * x)
{
    int l,r;

    if (x == &T->nil) {
        return 0;
    }

    l = _rbt_dept(T,x->l);
    r = _rbt_dept(T,x->r);

    if (l > r) {
        return l + 1;
    } else {
        return r + 1;
    }
}

int rbt_dept(rbtree_t * T)
{
    if (T) {
        return _rbt_dept(T,T->root);
    }
    return 0;
}
