

/*
 * CopyLeft (C) nie950@gmail.com
 */


#ifndef __ATOMIC_H_INCLUDED__
#define __ATOMIC_H_INCLUDED__

#define HAVE_ATOMIC_OPS   1

typedef int32_t                     atomic_int_t;
typedef uint32_t                    atomic_uint_t;
typedef volatile     atomic_uint_t  atomic_t;

#define ATOMIC_T_LEN               (sizeof("-2147483648") - 1)


#ifdef HAVE_OS_WIN32

#if defined( __WATCOMC__ ) || defined( __BORLANDC__ ) || ( _MSC_VER >= 1300 )

/* the new SDK headers */

#define atomic_cmp_set(lock, old, set)                                    \
     ((atomic_uint_t) InterlockedCompareExchange((long *) lock, set, old) \
                          == old)

#else

/* the old MS VC6.0SP2 SDK headers */

#define atomic_cmp_set(lock, old, set)                                    \
     (InterlockedCompareExchange((void **) lock, (void *) set, (void *) old)  \
      == (void *) old)

#define memory_barrier()

#endif

#define atomic_fetch_add(p, add) InterlockedExchangeAdd((long *) p, add)

#else 

#define atomic_cmp_set(lock, old, set)                                    \
    __sync_bool_compare_and_swap(lock, old, set)

#define atomic_fetch_add(value, add)                                      \
    __sync_fetch_and_add(value, add)

#endif


#if defined( __BORLANDC__ ) || ( __WATCOMC__ < 1230 )

/*
 * Borland C++ 5.5 (tasm32) and Open Watcom C prior to 1.3
 * do not understand the "pause" instruction
 */

#define cpu_pause()
#else
#define cpu_pause()       __asm { pause }
#endif


void spinlock_in(atomic_t *lock, atomic_int_t value, uint32_t spin);

#define spin_trylock(lock)  (*(lock) == 0 && atomic_cmp_set(lock, 0, 1))
#define spin_unlock(lock)    *(lock) = 0
#define spin_lock(lock)       spinlock_in(lock,1,2048);

#endif /* _NGX_ATOMIC_H_INCLUDED_ */
