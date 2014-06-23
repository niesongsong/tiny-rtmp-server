
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

int set_nonblocking(socket_t s)
{
    unsigned long  nb = 1;

    return ioctlsocket(s, FIONBIO, &nb);
}

int set_blocking(socket_t s)
{
    unsigned long  nb = 0;

    return ioctlsocket(s, FIONBIO, &nb);
}
