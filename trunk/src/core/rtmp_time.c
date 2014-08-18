
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


volatile uint64_t rtmp_current_msec;
volatile uint32_t rtmp_current_sec;


#ifdef HAVE_OS_WIN32

#define gettimeofday(x,y) rtmp_gettimeofday(x)

static void rtmp_gettimeofday(struct timeval *tp)
{
    uint64_t  intervals;
    FILETIME  ft;

    GetSystemTimeAsFileTime(&ft);

    intervals = ((uint64_t) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    intervals -= 116444736000000000;

    tp->tv_sec = (long) (intervals / 10000000);
    tp->tv_usec = (long) ((intervals % 10000000) / 10);
}

#endif

void rtmp_time_update(void)
{
    struct timeval   tv;
    uint32_t sec,msec;

    gettimeofday(&tv,NULL);

    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    rtmp_current_sec = sec;
    rtmp_current_msec = (uint64_t) sec * 1000 + msec;
}
