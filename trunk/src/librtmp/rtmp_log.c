
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "librtmp_in.h"

#define RTMP_LOG_ERR            ( 0)
#define RTMP_LOG_WARNING        ( 1)
#define RTMP_LOG_INFO           ( 2)
#define RTMP_LOG_NORMAL         ( 3)
#define RTMP_LOG_DEBUG          ( 4)
#define RTMP_LOG_MAX            (RTMP_LOG_DEBUG+1)

static const char *log_level_string[] = {
    "error",
    "warning",
    "info",
    "normal",
    "debug",
    "unknown"
};

static char  log_default_file[256] = "rtmp_core.log";
static int   log_default_level = RTMP_LOG_ERR;
static char  log_str[8000];

void rtmp_log_core(const char *func,long line, int level,
    const char * fmt,...)
{
    va_list     ap;
    time_t      t;
    struct tm  *now;
    char        ts[128];
    FILE       *fp;

    if (level < RTMP_LOG_ERR || level > log_default_level) {
        return;
    }

    va_start(ap,fmt);
    t = (time_t)(rtmp_current_sec);
    now = localtime(&t);

    sprintf(ts,"%04d-%02d-%02d %02d:%02d:%02d",
        now->tm_year+1900,now->tm_mon+1,now->tm_mday,
        now->tm_hour,now->tm_min,now->tm_sec);

    if (_vsnprintf(log_str,_array_length(log_str),fmt,ap) == -1) {
        va_end(ap);
        return ;
    }

    fp = fopen(log_default_file,"a+");
    if (fp == NULL) {
        va_end(ap);
        return ;
    }

/*#if HAVE_DEBUG*/
#if 0
    fprintf(fp,"[%s][%s():%d][%s]%s\n",ts,func,(int)line,
        log_level_string[level],log_str);
#else
    fprintf(fp,"[%s][%s]%s\n",ts,log_level_string[level],log_str);
#endif

    va_end(ap);
    fclose(fp);

    return;
}

int rtmp_log_init(int level,const char* logname)
{
    log_default_level = level;
    strncpy(log_default_file,logname,sizeof(log_default_file)-1);

    return 0;
}