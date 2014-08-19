
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __TIMER_H_INCLUDED__
#define __TIMER_H_INCLUDED__

void rtmp_event_timer_expire(rbtree_t *timer);
int rtmp_timer_compare(rbnode_t *a,rbnode_t *b);

#endif