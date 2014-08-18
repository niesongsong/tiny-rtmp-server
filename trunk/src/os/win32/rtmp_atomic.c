
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


void spinlock_in(atomic_t *lock, atomic_int_t value, uint32_t spin)
{
    uint32_t  i, n;

    for ( ;; ) {

        if (*lock == 0 && atomic_cmp_set(lock, 0, value)) {
            return;
        }

        for (n = 1; n < spin; n <<= 1) {
            for (i = 0; i < n; i++) {
                cpu_pause();
            }
            if (*lock == 0 && atomic_cmp_set(lock, 0, value)) {
                return;
            }
        }
        sched_yield();
    }
    return ;
}
