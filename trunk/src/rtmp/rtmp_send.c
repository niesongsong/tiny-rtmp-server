
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

int rtmp_send_n(int sockfd,const u_char *buf,int n)
{
    int sent,rc;

    sent = 0;
    while (n > 0) {
        rc = send(sockfd,(const char*)buf+sent,n,0);

        if (rc <= 0) {
            break;
        }

        sent += rc;
        n -= sent;
    }

    return sent;
}


int32_t rtmp_send_buf(int sockfd,mem_buf_t *wbuf)
{
    int32_t  r,n;

    while (wbuf->last != wbuf->end) {

        n = wbuf->end - wbuf->last;
        r = send(sockfd,(char *)wbuf->last,n,0);

        if (r == -1 || r == 0) {
            if (sock_errno == SOCK_EAGAIN) {
                return SOCK_EAGAIN;
            }
            return SOCK_ERROR;
        }

        wbuf->last += r;
    }
    return SOCK_OK;
}
