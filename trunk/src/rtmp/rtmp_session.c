
/*
 * CopyLeft (C) nie950@gmail.com
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
    mem_pool_t          *temp_pool;
    rtmp_message_t      *msg;

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

    temp_pool = c->listening->cycle->temp_pool;

    rtmp_core_handshake_init();

    pool = c->pool;
    
    ssz = RTMP_DEFAULT_MAX_STREAMS * sizeof(rtmp_chunk_stream_t *);
    lsz = (RTMP_DEFAULT_MAX_LIVES + 1) * sizeof(rtmp_live_stream_t *);

    session = mem_pcalloc(pool,sizeof(rtmp_session_t) + ssz + lsz);
    if (session == NULL) {
        return NULL;
    }

    session->chunk_streams = (rtmp_chunk_stream_t **)((char *)session 
        + sizeof(rtmp_session_t));
    session->max_streams = RTMP_DEFAULT_MAX_STREAMS;

    session->lives = (rtmp_live_link_t**)((char *)session->chunk_streams + ssz);
    session->max_lives = RTMP_DEFAULT_MAX_LIVES;
    for (i = 0;i < session->max_lives;i++) {
        session->lives[i] = RTMP_NULL;
    }

    session->handshake = rtmp_handshake_alloc(pool);
    if (session->handshake == NULL) {
        return NULL;
    }

    session->ack_window = RTMP_DEFAULT_ACK;
    session->ping_timeout = RTMP_DEFAULT_PING;

    session->in_chunk_size = RTMP_DEFAULT_IN_CHUNKSIZE;
    session->in_chain = NULL;

    session->out_chunk_size = RTMP_DEFAULT_OUT_CHUNKSIZE;
    session->out_queue = c->listening->cycle->out_queue;
    
    msg = mem_pcalloc(pool,session->out_queue * sizeof(rtmp_message_t));
    if (msg == NULL) {
        return NULL;
    }

    session->out_message = msg;
    for (i = 0;i < session->out_queue;i++) {
        msg[i].head.buf  = msg[i].head_buf;
        msg[i].head.end  = msg[i].head_buf + sizeof(msg[i].head_buf);
        msg[i].head.last = msg[i].head.end;
    }
    
    session->c = c;
    session->pool = c->pool; 
    session->sid = c->fd;

    session->temp_pool = temp_pool;
    session->chunk_time = -1;

    rtmp_log(RTMP_LOG_INFO,"[%d]create session",session->sid);

    return session;
}

int32_t rtmp_session_destroy(rtmp_session_t * session)
{
    uint32_t                i;
    rtmp_connection_t      *c;
    rtmp_live_link_t      **lives;

    if (session == NULL) {
        return RTMP_FAILED;
    }

    rtmp_log(RTMP_LOG_INFO,"[%d]destroy session",session->sid);
    c = session->c;

    /*free chains*/
    for (i = 0;i < session->out_queue;i++) {
        if (session->out_message[i].chain != NULL) {
            rtmp_core_free_chains(session,session->chunk_pool,
                session->out_message[i].chain);
        }
    }

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