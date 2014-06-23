

/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __RTMP_SESSION_H_INCLUDED__
#define __RTMP_SESSION_H_INCLUDED__

struct rtmp_session_s {
    rtmp_handshake_t     handshake;
    rtmp_connection_t   *c;
    mem_pool_t          *pool; /* c->pool */

    char                *tcURL;
    char                *url;
    char                *host;
    char                *app;

    uint32_t             ping;  /*ping*/
};

rtmp_session_t *rtmp_session_create(rtmp_connection_t *);
int32_t rtmp_session_destroy(rtmp_session_t *);


#endif