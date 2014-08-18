

/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

rtmp_session_t *rtmp_session_create(rtmp_connection_t *c)
{
    mem_pool_t          *pool;
    rtmp_session_t      *session;
    socklen_t            socklen;
    struct sockaddr_in  *addr_in;
    uint32_t             ssz,lsz,i;

    /*get local address*/
    socklen = sizeof(c->local_sockaddr);
    if (getsockname(c->fd, (struct sockaddr *) &c->local_sockaddr, 
        &socklen) == -1) 
    {
        rtmp_log(RTMP_LOG_WARNING,"getsockname() failed!");
        return NULL;
    }

    switch(c->local_sockaddr.sa_family) {
    case AF_INET:
        addr_in = (struct sockaddr_in *)&c->local_sockaddr;
        sprintf(c->local_addr_text,"%s",inet_ntoa(addr_in->sin_addr));

        break;
    case AF_UNIX:
        break;
    }

    if (c->pool == NULL) {
        c->pool = mem_create_pool(MEM_DEFAULT_POOL_SIZE);
        if (c->pool == NULL) {
            return NULL;
        }
    }

    rtmp_core_handshake_init();

    pool = c->pool;
    
    ssz = RTMP_DEFAULT_MAX_STREAMS * sizeof(rtmp_chunk_stream_t *);
    lsz = RTMP_DEFAULT_MAX_LIVES * sizeof(rtmp_live_stream_t *);

    session = mem_pcalloc(pool,sizeof(rtmp_session_t) + ssz + lsz);
    if (session == NULL) {
        return NULL;
    }

    session->streams = (rtmp_chunk_stream_t **)((char *)session 
        + sizeof(rtmp_session_t));
    session->max_streams = RTMP_DEFAULT_MAX_STREAMS;

    session->lives = (rtmp_live_link_t**)((char *)session->streams + ssz);
    session->max_lives = RTMP_DEFAULT_MAX_LIVES;
    for (i = 0;i < session->max_lives;i++) {
        session->lives[i] = RTMP_NULL;
    }

    if (rtmp_handshake_alloc(pool,&session->handshake) != RTMP_OK) {
        return NULL;
    }

    session->ack_window = RTMP_DEFAULT_ACK;
    session->ping = RTMP_DEFAULT_PING;

    session->in_chunk_size = RTMP_DEFAULT_IN_CHUNKSIZE;
    session->in_chain = NULL;

    session->out_chunk_size = RTMP_DEFAULT_OUT_CHUNKSIZE;
    session->out_queue = c->listening->cycle->out_queue;
    session->out_message = mem_pcalloc(pool,session->out_queue);
    if (session->out_message == NULL) {
        return NULL;
    }

    session->c = c;
    session->pool = c->pool;
    session->temp_pool = c->listening->cycle->temp_pool;
    session->sid = c->fd;

    session->stream_time = -1;
    session->last_stream = -1;

    rtmp_log(RTMP_LOG_INFO,"[%d]create session",session->sid);

    return session;
}

int32_t rtmp_session_destroy(rtmp_session_t * session)
{
    uint32_t                i;
    rtmp_connection_t      *c;
    rtmp_live_link_t     **lives;
    
    if (session == NULL) {
        return RTMP_FAILED;
    }

    rtmp_log(RTMP_LOG_INFO,"[%d]destroy session",session->sid);
    c = session->c;

    /*clear live streams*/
    lives = session->lives;
    for (i = 1;i < session->max_lives;i++) {

        if ((lives[i] != RTMP_NULL) && (lives[i] != RTMP_READY)) {
            rtmp_app_live_release(lives[i]);
        }
    }

    if (c) {
        if (c->pool) {
            mem_reset_pool(c->pool);
        }

        close_connection(session->c);
    }

    return RTMP_OK;
}