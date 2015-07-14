
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __LIB_RTMP_LOG_H_INCLUDED__
#define __LIB_RTMP_LOG_H_INCLUDED__


#define rtmp_log(l,...)                                    \
    rtmp_log_core(__FUNCTION__,__LINE__,l,__VA_ARGS__)

int rtmp_log_init(int level,const char* logname);
void rtmp_log_core(const char *func,long line, int level,
    const char * fmt,...);


#endif