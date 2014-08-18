
/*
 * Copyright (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

#define RTMP_LOG_DEFAULT_LEVEL   RTMP_LOG_DEBUG  
#define RTMP_LOG_DEFAULT_SIZE    (10*MB)

#define LOG_MAX (RTMP_LOG_DEBUG+1)

#define rtmp_default_log    "rtmp_core.log"
#define ARRSIZE(x)          (sizeof(x)/sizeof(x[0]))

typedef struct rtmp_log_s rtmp_log_t;

struct rtmp_log_s {
    int          level;
    size_t       maxsize;
    char         file[LOG_MAX][512];
    FILE        *fp[LOG_MAX];
};

static char * rtmp_log_level[] = {
    "error",
    "warning",
    "info",
    "normal",
    "debug"
};

static rtmp_log_t log_file = {
    RTMP_LOG_DEFAULT_LEVEL,
    RTMP_LOG_DEFAULT_SIZE,
    {{0}},
    {0}
};

static char log_str[8000] = {0};

#ifndef HAVE_OS_WIN32
#define  _vsnprintf  vsnprintf
#endif

void rtmp_log_core(const char *func,long line, int level,
    const char * fmt,...)
{
    va_list ap;
    time_t t;
    struct tm * now;
    char ts[128];
    FILE *fp;

    if (level < RTMP_LOG_ERR || level > log_file.level) {
        return;
    }

    va_start(ap,fmt);
    t = (time_t)(rtmp_current_sec);
    now = localtime(&t);

    sprintf(ts,"%04d-%02d-%02d %02d:%02d:%02d",                 \
        now->tm_year+1900,now->tm_mon+1,now->tm_mday,           \
        now->tm_hour,now->tm_min,now->tm_sec);

    if (_vsnprintf(log_str,ARRSIZE(log_str),fmt,ap) == -1) {
        va_end(ap);
        return ;
    }

    fp = log_file.fp[level];
    if (fp == NULL) {

        if (strcmp(log_file.file[level],"") == 0) {
            fp = stdout;
        } else {

#ifndef HAVE_OS_WIN32
            int l;

            for (l = RTMP_LOG_ERR;l <= RTMP_LOG_DEBUG;l++) {
                if (l == level) {
                    continue;
                }
                if (strcmp(log_file.file[level],log_file.file[l]) == 0) {
                    if (log_file.fp[l] != NULL) {
                        fp = log_file.fp[l];
                        break;
                    }
                }
            }

            if (fp == NULL) {
                fp = fopen(log_file.file[level],"a+");
            }

#else
            fp = stdout;
#endif
        }

        if (fp == NULL) {
            va_end(ap);
            return;
        }
        log_file.fp[level] = fp;
    }

#if 0
    fprintf(fp,"[%s][%s():%d][%s]%s\n",                 \
        ts,func,(int)line,                              \
        rtmp_log_level[level],log_str);

#else

    fprintf(fp,"[%s][%s]%s\n",                 \
        ts,rtmp_log_level[level],log_str);

#endif

    va_end(ap);

    return;
}

int rtmp_log_init(int level,const char* logname)
{
    if ((logname) == NULL || (level < 0) || (level >= LOG_MAX)) {
        return RTMP_FAILED;
    }

    log_file.level = level;
    do {
        strncpy(log_file.file[level],logname,511);
    } while ( --level >= 0);

#ifdef HAVE_DEBUG
    rtmp_log(RTMP_LOG_INFO,"log file:[%d]%s",level,logname);
#endif

    return RTMP_OK;
}

void rtmp_log_set_level(int level)
{
    if ((level < 0) || (level > LOG_MAX)) {
        return ;
    }

    log_file.level = level;
    rtmp_log(RTMP_LOG_INFO,"set level:[%s]",log_file.file[level]);

    return ;
}

/*max size (B)*/
void rtmp_log_set_maxsize(size_t size)
{
    if (size > 10*MB) {
        return ;
    }

    log_file.maxsize = size;
    rtmp_log(RTMP_LOG_INFO,"set size:[%d]",size);

    return ;
}

#ifndef HAVE_OS_WIN32
#define _base_log_printf printf
#else
#define _base_log_printf OutputDebugStringA
#endif

void rtmp_log_printf(const char * fmt,...)
{
    va_list ap;
    char buf[1024];

    va_start(ap,fmt);

    _vsnprintf(buf,1024,fmt,ap);
    _base_log_printf(buf);

    va_end(ap);
}
