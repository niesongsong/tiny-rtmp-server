
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


#define _link_init(n)                                                       \
    ((n)->next = (n)->prev = (n))

#define _link_remove(n)                                                     \
    (n)->next->prev = (n)->prev;                                            \
    (n)->prev->next = (n)->next;

#define _link_insert(a,b)                                                   \
    (b)->prev = (a);                                                        \
    (b)->next = (a)->next;                                                  \
    (b)->prev->next = (b);                                                  \
    (b)->next->prev = (b);

#define _link_num(l,i)                                                      \
    do {                                                                    \
        const link_t * it; (i)=0;                                           \
        for (it = (l)->next; it != (l); it = it->next, (i)++) {/*void*/}    \
    } while (0)

void list_init(list_t * l)
{
    _link_init(l);
}

void list_insert_tail(list_t * l,link_t * n)
{
    _link_insert(l->prev,n);
}

void list_insert_head(list_t * l,link_t * n)
{
    _link_insert(l,n);
}

void list_remove(link_t * n)
{
    _link_remove(n);
}

int  list_empty(list_t *l)
{
    return l == l->next;
}

int list_num(list_t * l)
{
    int i;

    _link_num(l,i);

    return i;
}

link_t * list_find(list_t *l,compare proc,void * u)
{
    link_t *it;

    for (it = l->next; it != l; it = it->next) {
        if (proc(it,u) == 0) {
            return it;
        }
    }

    return 0;
}

void queue_init(queue_t * q)
{
    _link_init(q);
}

void queue_push(queue_t * q,link_t *n)
{
    _link_insert(q->next,n);
}

link_t * queue_pop(queue_t * q)
{
    link_t * n;

    if (q->next == q) {
        return 0;
    }

    n = q->next;

    _link_remove(n);

    return n;
}

link_t * queue_front(queue_t * q)
{
    if (q->next == q) {
        return 0;
    }

    return q->next;
}

link_t * queue_rear(queue_t * q)
{
    if (q->next == q) {
        return 0;
    }

    return q->prev;
}

int queue_length(queue_t * q)
{
    int i = 0;

    _link_num(q,i);

    return i;
}

void stack_init(stack_t *st)
{
    _link_init(st); 
}

void stack_push(stack_t *st,link_t *n)
{
    _link_insert(st->next,n)
}

link_t * stack_pop(stack_t *st)
{
    link_t *n;

    n = st->prev;

    if (n == st) {
        return 0;
    }

    _link_remove(n);

    return n;
}

int stack_dept(stack_t *st)
{
    int i = 0;

    _link_num(st,i);

    return i;
}
