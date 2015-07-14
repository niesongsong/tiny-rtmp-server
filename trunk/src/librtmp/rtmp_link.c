
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "librtmp_in.h"

void list_init(list_t* l) 
{
    l->next = l->prev = l;
}

void list_insert_head(list_t* l,link_t* n)
{
    n->next = l->next;
    n->prev = l;

    n->prev->next = n;
    n->next->prev = n;
}

void list_insert_tail(list_t* l,link_t* n)
{
    n->next = l;
    n->prev = l->prev;

    n->prev->next = n;
    n->next->prev = n;
}

void list_remove(link_t* n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

#ifdef HAVE_DEBUG
    n->next = n->prev = n;
#endif

}