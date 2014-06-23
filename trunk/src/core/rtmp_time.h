
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __TIME_H_INCLUDED__
#define __TIME_H_INCLUDED__

extern volatile uint64_t rtmp_current_msec;
extern volatile uint32_t rtmp_current_sec;

void rtmp_time_update(void);

#endif