
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

#define ERR_MAX 1000

char    *sys_error_unknown = "Unknown error";
char   **sys_error_list;
int32_t  sys_error_len;
char    *rtmp_error_list[32];

char *rtmp_strncpy(char* dest,const char* src, size_t size)
{
    if (!dest || !src) {
        return 0;
    }

    while (((int32_t)--size > 0) && (*dest++ = *src++)) {}
    dest[0] = '\0';

    return dest;
}


char* rtmp_strerror(int32_t err, char *errstr, size_t size)
{
    char *msg;

    if (err < 0 && err >= sys_error_len) {
        return 0;
    }

    msg = sys_error_list[err];
    rtmp_strncpy(errstr,msg,size);

    return errstr;
}


int32_t rtmp_strerror_init(void)
{
    int32_t err,len;
    char * msg,*p;
    size_t slen;

    for (err = 0;err < ERR_MAX;err++) {

        errno = 0;
        msg = strerror(err);

        if (errno == EINVAL || msg == NULL) {
            break;
        }
    }

    if (err == 0) {
        goto failed;
    }

    len = err;
    sys_error_list = malloc(sizeof(char *)*len);
    if (sys_error_list == NULL) {
        goto failed;
    }
    
    for (err = 0;err < len;err++) {

        msg = strerror(err);
        slen = strlen(msg);

        p = malloc(slen+1);
        if (p == NULL) {
            goto failed;
        }

        memcpy(p,msg,slen);
        p[slen] = '\0';

        sys_error_list[err] = p;
    }

    sys_error_len = len;

    return RTMP_OK;

failed:

    return RTMP_FAILED;
}
