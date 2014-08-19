
/*
 * CopyLeft (C) nie950@gmail.com
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
    mem_buf_chain_t     *chain;
};

struct mem_buf_s {
    u_char    *buf;
    u_char    *last;
    u_char    *end;
};

struct mem_buf_chain_s {
    mem_buf_t        chunk;
    uint32_t         chunk_size;
    uint32_t         locked;
    mem_buf_chain_t *next;
};

#define mem_malloc                     malloc
#define mem_free                       free
#define mem_memalign(alignment, size)  mem_alloc(size)

void *mem_alloc(size_t size);
void *mem_calloc(size_t size);

void *mem_prealloc(mem_pool_t *pool,void *ptr,size_t old,size_t new);
void *mem_pcrealloc(mem_pool_t *pool,void *ptr,size_t old,size_t new);

mem_pool_t *mem_create_pool(size_t size);
void mem_destroy_pool(mem_pool_t *pool);
void mem_reset_pool(mem_pool_t *pool);

mem_buf_t* mem_buf_palloc(mem_pool_t *pool,mem_buf_t *buf,size_t size);
mem_buf_t* mem_buf_pcalloc(mem_pool_t *pool,mem_buf_t *buf,size_t size);

mem_buf_t* mem_buf_merge_chain(mem_buf_chain_t *chain,mem_pool_t *temp);

void *mem_palloc(mem_pool_t *pool, size_t size);
void *mem_pnalloc(mem_pool_t *pool, size_t size);
void *mem_pcalloc(mem_pool_t *pool, size_t size);
void *mem_pmemalign(mem_pool_t *pool, size_t size, size_t alignment);
int mem_pfree(mem_pool_t *pool, void *p);

mem_buf_chain_t* mem_alloc_chain_link(mem_pool_t *pool);

#define mem_free_chain_link(pool,cl) \
    cl->next = pool->chain;          \
    pool->chain = cl;

char *mem_dup_str(char *src,mem_pool_t *pool);

#endif