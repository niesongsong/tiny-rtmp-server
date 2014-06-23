
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

extern volatile uint64_t rtmp_current_msec;

void rtmp_event_timer_expire(rbtree_t *timer)
{
    rbnode_t *x;
    rtmp_event_t *ev;

    while ((x = rbt_min(timer,timer->root)) != 0) {

        if (x->k > rtmp_current_msec) {
            break;
        }

        rbt_remove(timer,x);
        ev = struct_entry(x,rtmp_event_t,timer);

        ev->timeout = 1;

        if (ev->handler) {
            ev->handler(ev);
        }
    }

    return ;
}

int rtmp_timer_compare(rbnode_t *a,rbnode_t *b)
{
    rtmp_event_t *eva,*evb;
    int64_t rc;

    eva = struct_entry(a,rtmp_event_t,timer);
    evb = struct_entry(b,rtmp_event_t,timer);

    rc = (eva->timer.k - evb->timer.k);

    if (rc == 0) {
        return 0;
    }

    if (rc > 0) {
        return 1;
    }

    return -1;
}