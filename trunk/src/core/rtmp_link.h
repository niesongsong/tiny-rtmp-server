
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __LINK_H_INCLUDED__
#define __LINK_H_INCLUDED__

typedef struct link_s link_t;
typedef struct link_s list_t;
typedef struct link_s queue_t;
typedef struct link_s stack_t;

struct link_s {
    struct link_s * prev;
    struct link_s * next;
};

#define struct_entry(p,t,m)                                 \
    ((t *)((char *)(p)-(char *)(&((t *)0)->m))) 

typedef int (*compare)(link_t *link,void *data);

/*list interface*/
void list_init(list_t * l);
void list_insert_head(list_t * l,link_t * n);
void list_insert_tail(list_t * l,link_t * n);
void list_remove(link_t * n);
int  list_empty(list_t *l);

int  list_num(list_t * l);

link_t * list_find(list_t *l,compare,void *);

/*queue interface*/
void queue_init(queue_t * q);
void queue_push(queue_t * q,link_t *n);
link_t * queue_pop(queue_t * q);

link_t * queue_front(queue_t * q);
link_t * queue_rear(queue_t * q);

int queue_length(queue_t * q);

/*stack interface*/
void stack_init(stack_t *st);
void stack_push(stack_t *st,link_t *n);
link_t * stack_pop(stack_t *st);
int stack_dept(stack_t *st);

#endif