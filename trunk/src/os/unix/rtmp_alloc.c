
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

uint32_t  mem_pagesize;
uint32_t  mem_pagesize_shift;
uint32_t  mem_cacheline_size;

static void *mem_palloc_block(mem_pool_t *pool, size_t size);
static void *mem_palloc_large(mem_pool_t *pool, size_t size);

void *mem_alloc(size_t size)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        rtmp_log(RTMP_LOG_WARNING, "malloc(%u) failed", size);
    }

    return p;
}

void *mem_calloc(size_t size)
{
    void  *p;

    p = mem_alloc(size);

    if (p) {
        memset(p, 0, size);
    }

    return p;
}

mem_pool_t *mem_create_pool(size_t size)
{
    mem_pool_t  *p;

    p = mem_memalign(MEM_POOL_ALIGNMENT, size);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(mem_pool_t);
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(mem_pool_t);
    p->max = (size < MEM_MAX_ALLOC_FROM_POOL) ? size : MEM_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->large = NULL;
    p->chain = NULL;

    return p;
}

void mem_destroy_pool(mem_pool_t *pool)
{
    mem_pool_t          *p, *n;
    mem_pool_large_t    *l;

    for (l = pool->large; l; l = l->next) {

        rtmp_log(RTMP_LOG_DEBUG, "free: %p", l->alloc);

        if (l->alloc) {
            mem_free(l->alloc);
        }
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        mem_free(p);

        if (n == NULL) {
            break;
        }
    }
}

void mem_reset_pool(mem_pool_t *pool)
{
    mem_pool_t        *p;
    mem_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            mem_free(l->alloc);
        }
    }

    pool->large = NULL;
    pool->chain = NULL;

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(mem_pool_t);
    }
}

void *mem_palloc(mem_pool_t *pool, size_t size)
{
    u_char      *m;
    mem_pool_t  *p;

    if (size <= pool->max) {

        p = pool->current;

        do {
            m = mem_align_ptr(p->d.last, MEM_ALIGNMENT);

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return mem_palloc_block(pool, size);
    }

    return mem_palloc_large(pool, size);
}


void *mem_pnalloc(mem_pool_t *pool, size_t size)
{
    u_char      *m;
    mem_pool_t  *p;

    if (size <= pool->max) {

        p = pool->current;

        do {
            m = p->d.last;

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return mem_palloc_block(pool, size);
    }

    return mem_palloc_large(pool, size);
}


static void *
mem_palloc_block(mem_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    mem_pool_t  *p, *new, *current;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = mem_memalign(MEM_POOL_ALIGNMENT, psize);
    if (m == NULL) {
        return NULL;
    }

    new = (mem_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(mem_pool_data_t);
    m = mem_align_ptr(m, MEM_ALIGNMENT);
    new->d.last = m + size;

    current = pool->current;

    for (p = current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            current = p->d.next;
        }
    }

    p->d.next = new;

    pool->current = current ? current : new;

    return m;
}



static void *
mem_palloc_large(mem_pool_t *pool, size_t size)
{
    void            *p;
    uint32_t         n;
    mem_pool_large_t  *large;

    p = mem_alloc(size);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = mem_palloc(pool, sizeof(mem_pool_large_t));
    if (large == NULL) {
        mem_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


void *
ngx_pmemalign(mem_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    mem_pool_large_t  *large;

    p = mem_memalign(alignment, size);
    if (p == NULL) {
        return NULL;
    }

    large = mem_palloc(pool, sizeof(mem_pool_large_t));
    if (large == NULL) {
        mem_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


int ngx_pfree(mem_pool_t *pool, void *p)
{
    mem_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            rtmp_log(RTMP_LOG_DEBUG,"free: %p", l->alloc);

            mem_free(l->alloc);
            l->alloc = NULL;

            return RTMP_OK;
        }
    }

    return RTMP_FAILED;
}

void * mem_pcalloc(mem_pool_t *pool, size_t size)
{
    void *p;

    p = mem_palloc(pool, size);
    if (p) {
        memset(p, 0, size);
    }

    return p;
}

mem_buf_chain_t* mem_alloc_chain_link(mem_pool_t *pool)
{
    mem_buf_chain_t  *cl;

    cl = pool->chain;

    if (cl) {
        pool->chain = cl->next;
        return cl;
    }

    cl = mem_pcalloc(pool, sizeof(mem_buf_chain_t));
    if (cl == NULL) {
        return NULL;
    }

    return cl;
}


char *mem_dup_str(char *src,mem_pool_t *pool)
{
    size_t  len;
    char   *dst;

    len = strlen(src);
    dst = mem_pcalloc(pool,len + 1);

    if (dst && len > 0) {
        memcpy(dst,src,len);
    }

    return dst;
}

mem_buf_t* mem_buf_palloc(mem_pool_t *pool,mem_buf_t *buf,size_t size)
{
    if (buf != NULL) {
        buf->buf = mem_palloc(pool,sizeof(mem_buf_t) + size);
    } else {
        buf = mem_palloc(pool,sizeof(mem_buf_t) + size);
        if (buf) {
            buf->buf = (unsigned char*)buf + sizeof(mem_buf_t);
        }
    }

    if (buf->buf == NULL) {
        return NULL;
    }

    buf->last = buf->buf;
    buf->end = buf->buf + size;

    return buf;
}

mem_buf_t* mem_buf_pcalloc(mem_pool_t *pool,mem_buf_t *buf,size_t size)
{
    if (buf != NULL) {
        buf->buf = mem_pcalloc(pool,sizeof(mem_buf_t) + size);
    } else {
        buf = mem_pcalloc(pool,sizeof(mem_buf_t) + size);
        if (buf) {
            buf->buf = (unsigned char*)buf + sizeof(mem_buf_t);
        }
    }

    if (buf->buf == NULL) {
        return NULL;
    }

    buf->last = buf->buf;
    buf->end = buf->buf + size;

    return buf;
}

