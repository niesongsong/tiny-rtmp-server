
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

int set_tcppush(socket_t s)
{
    int  tcp_nopush;

    tcp_nopush = 0;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
                      (const void *) &tcp_nopush, sizeof(int));
}
