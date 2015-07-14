
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __LIB_RTMP_LINK_H_INCLUDED__
#define __LIB_RTMP_LINK_H_INCLUDED__

typedef struct link_s link_t;
typedef struct link_s list_t;

struct link_s {
    struct link_s * prev;
    struct link_s * next;
};

#define struct_entry(p,t,m)                                 \
    ((t *)((char *)(p)-(char *)(&((t *)0)->m))) 

void list_init(list_t* l);
void list_insert_head(list_t* l,link_t* n);
void list_insert_tail(list_t* l,link_t* n);
void list_remove(link_t* n);

#endif