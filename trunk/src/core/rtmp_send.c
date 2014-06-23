
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
