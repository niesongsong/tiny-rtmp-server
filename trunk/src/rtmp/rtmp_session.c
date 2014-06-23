

/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

rtmp_session_t *rtmp_session_create(rtmp_connection_t *c)
{
    mem_pool_t       *pool;
    rtmp_session_t   *session;

    if (c->pool == NULL) {
        c->pool = mem_create_pool(MEM_DEFAULT_POOL_SIZE);
        if (c->pool == NULL) {
            return NULL;
        }
    }

    rtmp_core_handshake_init();

    pool = c->pool;
    session = mem_pcalloc(pool,sizeof(rtmp_session_t));
    if (session == NULL) {
        return NULL;
    }

    if (rtmp_handshake_alloc(pool,&session->handshake) != RTMP_OK) {
        return NULL;
    }

    session->c = c;
    session->pool = c->pool;

    return session;
}

int32_t rtmp_session_destroy(rtmp_session_t * session)
{
    rtmp_connection_t *c;

    c = session->c;

    if (session == NULL) {
        return RTMP_FAILED;
    }

    if (c) {
        if (c->pool) {
            mem_reset_pool(c->pool);
        }

        free_connection(session->c);
    }

    return RTMP_OK;
}