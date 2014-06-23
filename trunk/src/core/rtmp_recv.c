
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

int rtmp_recv_n(int sockfd,u_char *buf,int n)
{
    int recvd,rc;

    recvd = 0;
    while (n > 0) {
        rc = recv(sockfd,(char *)buf+recvd,n,0);

        if (rc <= 0) {
            break;
        }

        recvd += rc;
        n -= recvd;
    }

    return recvd;
}