
/*
 * CopyLeft (C) nie950@gmail.com
 */


#ifndef __ARRAY_H_INCLUDED__
#define __ARRAY_H_INCLUDED__


typedef struct {
    void        *elts;
    uint32_t   nelts;
    size_t       size;
    uint32_t   nalloc;
    mem_pool_t  *pool;
} array_t;


array_t *array_create(mem_pool_t *p, uint32_t n, size_t size);
void array_destroy(array_t *a);
void *array_push(array_t *a);
void *array_push_n(array_t *a, uint32_t n);
int array_init(array_t *array, mem_pool_t *pool, uint32_t n, size_t size);

#endif /* __ARRAY_H_INCLUDED__ */