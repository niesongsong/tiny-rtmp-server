
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __ALLOC_H_INCLUDED__
#define __ALLOC_H_INCLUDED__

#define MEM_DEFAULT_POOL_SIZE    (16 * 1024)
#define MEM_POOL_ALIGNMENT       (16)
#define MEM_MAX_ALLOC_FROM_POOL  (mem_pagesize - 1)

typedef struct mem_pool_s           mem_pool_t;
typedef struct mem_pool_large_s     mem_pool_large_t;
typedef struct mem_buf_s            mem_buf_t;
typedef struct mem_buf_chain_s      mem_buf_chain_t;

struct mem_pool_large_s {
    mem_pool_large_t    *next;
    void                *alloc;
};

typedef struct {
    u_char               *last;
    u_char               *end;
    mem_pool_t           *next;
    uint32_t              failed;
} mem_pool_data_t;

struct mem_pool_s {
    mem_pool_data_t      d;
    size_t                max;
    mem_pool_t           *current;
    mem_pool_large_t    *large;
};

struct mem_buf_s {
    u_char    *buf;

    u_char    *last;
    u_char    *end;
};

struct mem_buf_chain_s {
    uint32_t   num;
    mem_buf_t *next;
};

#define mem_malloc                     malloc
#define mem_free                       free
#define mem_memalign(alignment, size)  mem_alloc(size)

void *mem_alloc(size_t size);
void *mem_calloc(size_t size);

mem_pool_t *mem_create_pool(size_t size);
void mem_destroy_pool(mem_pool_t *pool);
void mem_reset_pool(mem_pool_t *pool);

void *mem_palloc(mem_pool_t *pool, size_t size);
void *mem_pnalloc(mem_pool_t *pool, size_t size);
void *mem_pcalloc(mem_pool_t *pool, size_t size);
void *mem_pmemalign(mem_pool_t *pool, size_t size, size_t alignment);
int mem_pfree(mem_pool_t *pool, void *p);

void *mem_palloc_buf(mem_pool_t *pool,size_t size);
void *mem_pcalloc_buf(mem_pool_t *pool,size_t size);

#endif