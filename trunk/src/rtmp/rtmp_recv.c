
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

int32_t rtmp_recv_n(int sockfd,u_char *buf,int n)
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

int32_t rtmp_recv_buf(int sockfd,mem_buf_t *rbuf)
{
    int32_t  r,n;

    while (rbuf->last != rbuf->end) {

        n = rbuf->end - rbuf->last;
        r = recv(sockfd,(char *)rbuf->last,n,0);

        if (r == -1 || r == 0) {
            if (sock_errno == SOCK_EAGAIN) {
                return SOCK_EAGAIN;
            }
            return SOCK_ERROR;
        }

        rbuf->last += r;
    }
    return SOCK_OK;
}