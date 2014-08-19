
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


int array_init(array_t *a, mem_pool_t *pool, uint32_t n, size_t size)
{
    a->nelts = 0;
    a->size = size;
    a->nalloc = n;
    a->pool = pool;

    a->elts = mem_palloc(pool, n * size);
    if (a->elts == NULL) {
        return RTMP_ERROR;
    }

    return RTMP_OK;
}

array_t *
array_create(mem_pool_t *p, uint32_t n, size_t size)
{
    array_t *a;

    a = mem_palloc(p, sizeof(array_t));
    if (a == NULL) {
        return NULL;
    }

    if (array_init(a, p, n, size) != RTMP_OK) {
        return NULL;
    }

    return a;
}


void
array_destroy(array_t *a)
{
    mem_pool_t  *p;

    p = a->pool;

    if ((u_char *) a->elts + a->size * a->nalloc == p->d.last) {
        p->d.last -= a->size * a->nalloc;
    }

    if ((u_char *) a + sizeof(array_t) == p->d.last) {
        p->d.last = (u_char *) a;
    }
}


void *
array_push(array_t *a)
{
    void        *elt, *new;
    size_t       size;
    mem_pool_t  *p;

    if (a->nelts == a->nalloc) {

        /* the array is full */

        size = a->size * a->nalloc;

        p = a->pool;

        if ((u_char *) a->elts + size == p->d.last
            && p->d.last + a->size <= p->d.end)
        {
            /*
             * the array allocation is the last in the pool
             * and there is space for new allocation
             */

            p->d.last += a->size;
            a->nalloc++;

        } else {
            /* allocate a new array */

            new = mem_palloc(p, 2 * size);
            if (new == NULL) {
                return NULL;
            }

            memcpy(new, a->elts, size);
            a->elts = new;
            a->nalloc *= 2;
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts++;

    return elt;
}


void *
array_push_n(array_t *a, uint32_t n)
{
    void        *elt, *new;
    size_t       size;
    uint32_t     nalloc;
    mem_pool_t  *p;

    size = n * a->size;

    if (a->nelts + n > a->nalloc) {

        /* the array is full */

        p = a->pool;

        if ((u_char *) a->elts + a->size * a->nalloc == p->d.last
            && p->d.last + size <= p->d.end)
        {
            /*
             * the array allocation is the last in the pool
             * and there is space for new allocation
             */

            p->d.last += size;
            a->nalloc += n;

        } else {
            /* allocate a new array */

            nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

            new = mem_palloc(p, nalloc * a->size);
            if (new == NULL) {
                return NULL;
            }

            memcpy(new, a->elts, a->nelts * a->size);
            a->elts = new;
            a->nalloc = nalloc;
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts += n;

    return elt;
}
