
/*
 * Copyright (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

int32_t rtmp_core_cycle(rtmp_session_t *session);

void rtmp_core_recv(rtmp_event_t *ev);
void rtmp_core_send(rtmp_event_t *ev);

int32_t rtmp_server_handshake(rtmp_session_t *session)
{
    rtmp_connection_t   *c;
    rtmp_event_t        *rev,*wev;

    c = session->c;
    rev = c->read;
    wev = c->write;

    rev->handler = rtmp_handshake_recv;
    wev->handler = rtmp_handshake_send;
    
    session->handshake.stage = RTMP_HANDSHAKE_SERVER_INIT;
    session->handshake.rbuf.end = session->handshake.rbuf.buf;

    rtmp_handshake_recv(rev);
    
    return RTMP_OK;
}

int32_t rtmp_client_handshake(rtmp_session_t *session)
{
    rtmp_connection_t   *c;
    rtmp_event_t        *rev,*wev;

    c = session->c;
    rev = c->read;
    wev = c->write;

    rev->handler = rtmp_handshake_recv;
    wev->handler = rtmp_handshake_send;

    session->handshake.stage = RTMP_HANDSHAKE_CLIENT_INIT;
    session->handshake.wbuf.end = session->handshake.wbuf.buf;

    rtmp_handshake_send(wev);

    return RTMP_OK;
}

void rtmp_core_init_connnect(rtmp_connection_t *conn)
{
    rtmp_session_t  *session;
    
    session = rtmp_session_create(conn);
    if (session == NULL) {
        rtmp_log(RTMP_LOG_ERR,"create session error!");
        free_connection(conn);
        return ;
    }

    conn->data = session;
    session->handshake.stage = RTMP_HANDSHAKE_SERVER_C0C1;

    rtmp_server_handshake(session);

    return;
}

int32_t rtmp_server_handshake_done(rtmp_session_t *session)
{
    return rtmp_core_cycle(session);
}

int32_t rtmp_client_handshake_done(rtmp_session_t *session)
{
    return RTMP_OK;
}

int32_t rtmp_core_cycle(rtmp_session_t *session)
{
    rtmp_event_t        *rev,*wev;
    rtmp_connection_t   *conn;

    conn = session->c;

    rev = conn->read;
    wev = conn->write;

    rev->handler = rtmp_core_recv;
    wev->handler = rtmp_core_send;

    rtmp_core_recv(rev);

    return RTMP_OK;
}

void rtmp_core_recv(rtmp_event_t *ev)
{

}

void rtmp_core_send(rtmp_event_t *ev)
{

}
