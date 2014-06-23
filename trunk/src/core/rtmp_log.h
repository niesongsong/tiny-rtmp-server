
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __RTMP_LOG_H__INCLUDED__
#define __RTMP_LOG_H__INCLUDED__

#define KB   (1024)
#define MB   (1024*KB)

#define RTMP_LOG_ERR             0
#define RTMP_LOG_WARNING         1
#define RTMP_LOG_INFO            2
#define RTMP_LOG_NORMAL          3
#define RTMP_LOG_DEBUG           4

/*
 * func,line,level
 */
#define rtmp_log(l,...)                                                \
        rtmp_log_core(__FUNCTION__,__LINE__,l,__VA_ARGS__)

/* log core
 * format
 * [2012-07-19 12:23:08][main():32][debug]hello world!
 */

int rtmp_log_init(int level,const char *logname);
void rtmp_log_core(const char *,long,int ,const char *,...);
void rtmp_log_set_level(int level);
void rtmp_log_set_maxsize(size_t size);
void rtmp_log_printf(const char *,...);

#endif