
/*
 * Copyright (C) nie950@gmail.com
 */

/*
 * see http://www.codeman.net/2014/06/518.html
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

/*[0,1)[digest][2,3)*/
#define handshake_make_digest(k,l,d,p)                              \
    do {                                                            \
        unsigned int hl;                                            \
        HMAC_Init_ex(&handshake_hmac, k, l, EVP_sha256(), NULL);    \
        if ((p)[1] > (p)[0]) {                                      \
            HMAC_Update(&handshake_hmac,(p)[0],(p)[1]-(p)[0]);      \
        }                                                           \
        if ((p)[3] > (p)[2]) {                                      \
            HMAC_Update(&handshake_hmac,(p)[2],(p)[3]-(p)[2]);      \
        }                                                           \
        HMAC_Final(&handshake_hmac,d,&hl);                          \
    } while(0)


static HMAC_CTX handshake_hmac;
static int handshake_hmac_initialized = 0;

static uint8_t server_public_key[] = {
    'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
    'F', 'l', 'a', 's', 'h', ' ', 'M', 'e', 'd', 'i', 'a', ' ',
    'S', 'e', 'r', 'v', 'e', 'r', ' ',
    '0', '0', '1',

    0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1,
    0x02, 0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
    0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};

static uint8_t client_public_key[] = {
    'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
    'F', 'l', 'a', 's', 'h', ' ', 'P', 'l', 'a', 'y', 'e', 'r', ' ',
    '0', '0', '1',

    0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1,
    0x02, 0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
    0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};


static const uint8_t rtmp_server_version[4] = {
    0x04, 0x05, 0x06, 0x01
};

static const uint8_t rtmp_client_version[4] = {
    0x80, 0x00, 0x07, 0x02
};

/*

 |client|              |server|

    | -----(C0, C1)---->  |
    | <----(S0, S1)-----  |
    | <----(  S2  )-----  |
    | -----(  C2  )---->  |

*/

static void handshake_calc_c1s1_digest(uint8_t *c1s1,uint8_t *key,int len);

/*client*/
static int32_t rtmp_handshake_prepare_send_c0c1(rtmp_session_t *session);
static int32_t rtmp_handshake_prepare_recv_s0s1(rtmp_session_t *session);
static int32_t rtmp_handshake_prepare_recv_s2(rtmp_session_t *session);
static int32_t rtmp_handshake_prepare_send_c2(rtmp_session_t *session);
static int32_t rtmp_handshake_verify_s2(rtmp_session_t *session);

/*server*/
static void rtmp_handshake_prepare_recv_c0c1(rtmp_session_t *session);
static int32_t rtmp_handshake_prepare_send_s0s1(rtmp_session_t *session);
static int32_t rtmp_handshake_prepare_send_s2(rtmp_session_t *session);
static int32_t rtmp_handshake_prepare_recv_c2(rtmp_session_t *session);
static int32_t rtmp_handshake_verify_c2(rtmp_session_t *session);


static uint32_t handshake_get_x(uint8_t *X1)
{
    return ((uint32_t)X1[0] + (uint32_t)X1[1] +
            (uint32_t)X1[2] + (uint32_t)X1[3]) % 728;
}

void rtmp_core_handshake_init()
{
    if (handshake_hmac_initialized == 0) {
        handshake_hmac_initialized = 1;
        HMAC_CTX_init(&handshake_hmac);
    }
}

void rtmp_handshake_recv(rtmp_event_t *ev)
{
    rtmp_session_t     *session;
    rtmp_connection_t  *conn;
    rtmp_handshake_t   *hs;
    mem_buf_t          *rbuf;
    int                 r,n;
    
    conn = ev->data;
    session = conn->data;
    hs = &session->handshake;

    if (ev->timeout) {
        rtmp_session_destroy(session);
        return;
    }

    if (ev->timer_set) {
        rtmp_event_del_timer(ev);
    }

    rbuf = & hs->rbuf;
    while (rbuf->last != rbuf->end) {

        n = rbuf->end - rbuf->last;
        r = recv(conn->fd,(char *)rbuf->last,n,0);

        if (r == -1) {
            if (sock_errno == SOCK_EAGAIN) {

                if (!ev->active) {
                    rtmp_event_add(ev,EVENT_READ);
                }
                rtmp_event_add_timer(ev,6000);
                return ;
            }

            rtmp_session_destroy(session);
            return;
        }

        rbuf->last += r;
    }

    if (ev->active) {
        rtmp_event_delete(ev,EVENT_READ);
    }

    switch (hs->stage) {
    case RTMP_HANDSHAKE_SERVER_INIT:

        hs->stage = RTMP_HANDSHAKE_SERVER_C0C1;
        rtmp_handshake_prepare_recv_c0c1(session);
        rtmp_handshake_recv(conn->read);

        break;

    case RTMP_HANDSHAKE_SERVER_C0C1:

        hs->stage = RTMP_HANDSHAKE_SERVER_S0S1;
        if (rtmp_handshake_prepare_send_s0s1(session) != RTMP_OK) {
            rtmp_session_destroy(session);
            break;
        }
        rtmp_handshake_send(conn->write);

        break;

    case RTMP_HANDSHAKE_SERVER_C2:

        hs->stage = RTMP_HANDSHAKE_SERVER_DONE;
        if (rtmp_handshake_verify_c2(session) != RTMP_OK) {
            hs->stage = RTMP_HANDSHAKE_FAILED;
            rtmp_session_destroy(session);
            break;
        }
        rtmp_server_handshake_done(session);

        break;

    case RTMP_HANDSHAKE_CLIENT_S0S1:

        hs->stage = RTMP_HANDSHAKE_CLIENT_S2;
        if (rtmp_handshake_prepare_recv_s2(session) != RTMP_OK) {
            rtmp_session_destroy(session);
            break;
        }
        rtmp_handshake_recv(conn->read);

        break;

    case RTMP_HANDSHAKE_CLIENT_S2:

        if (rtmp_handshake_verify_s2(session) != RTMP_OK) {
            hs->stage = RTMP_HANDSHAKE_FAILED;
            rtmp_session_destroy(session);
            break;
        }

        hs->stage = RTMP_HANDSHAKE_CLIENT_C2;
        if (rtmp_handshake_prepare_send_c2(session) != RTMP_OK) {
            rtmp_session_destroy(session);
            break;
        }
        rtmp_handshake_recv(conn->read);

        break;

    default:
        rtmp_log(RTMP_LOG_ERR,"unknown stage");
        rtmp_session_destroy(session);
        break;
    }

    return ;
}

void rtmp_handshake_send(rtmp_event_t *ev)
{
    rtmp_session_t     *session;
    rtmp_connection_t  *conn;
    rtmp_handshake_t   *hs;
    mem_buf_t          *wbuf;
    int                 r,n;

    conn = ev->data;
    session = conn->data;
    hs = & session->handshake;

    if (ev->timeout) {
        rtmp_session_destroy(session);
        return;
    }

    if (ev->timer_set) {
        rtmp_event_del_timer(ev);
    }

    wbuf = & hs->wbuf;
    while (wbuf->last != wbuf->end) {

        n = wbuf->end - wbuf->last;
        r = send(conn->fd,(char *)wbuf->last,n,0);

        if (r == -1) {
            if (sock_errno == SOCK_EAGAIN) {

                if (!ev->active) {
                    rtmp_event_add(ev,EVENT_WRITE);
                }
                rtmp_event_add_timer(ev,6000);

                return ;
            }

            rtmp_session_destroy(session);
            return;
        }

        wbuf->last += r;
    }

    if (ev->active) {
        rtmp_event_delete(ev,EVENT_WRITE);
    }

    switch (hs->stage) {

    case RTMP_HANDSHAKE_SERVER_S0S1:

        hs->stage = RTMP_HANDSHAKE_SERVER_S2;
        if (rtmp_handshake_prepare_send_s2(session) != RTMP_OK) {
            rtmp_session_destroy(session);
            break;
        }
        rtmp_handshake_send(conn->write);
        break;

    case RTMP_HANDSHAKE_SERVER_S2:

        hs->stage = RTMP_HANDSHAKE_SERVER_C2;
        if (rtmp_handshake_prepare_recv_c2(session) != RTMP_OK) {
            rtmp_session_destroy(session);
            break;
        }
        rtmp_handshake_recv(conn->read);

        break;

    case RTMP_HANDSHAKE_CLIENT_INIT:

        hs->stage = RTMP_HANDSHAKE_CLIENT_C0C1;
        if (rtmp_handshake_prepare_send_c0c1(session) != RTMP_OK) {
            rtmp_session_destroy(session);
            break;
        }
        rtmp_handshake_send(conn->write);

        break;

    case RTMP_HANDSHAKE_CLIENT_C0C1:

        hs->stage = RTMP_HANDSHAKE_CLIENT_S0S1;
        if (rtmp_handshake_prepare_recv_s0s1(session) != RTMP_OK) {
            rtmp_session_destroy(session);
            break;
        }
        rtmp_handshake_recv(conn->read);

        break;

    case RTMP_HANDSHAKE_CLIENT_C2:

        hs->stage = RTMP_HANDSHAKE_SERVER_DONE;
        rtmp_client_handshake_done(session);

        break;

    default:
        rtmp_log(RTMP_LOG_ERR,"unknown stage");
        rtmp_session_destroy(session);
        break;
    }

    return ;
}

int32_t rtmp_handshake_alloc(mem_pool_t *pool,rtmp_handshake_t *handshake)
{
    mem_buf_t        *r,*w;
    u_char           *buf;

    memset(handshake,0,sizeof(sizeof(rtmp_handshake_t)));

    r = &handshake->rbuf;
    w = &handshake->wbuf;

    buf = mem_pcalloc(pool,HANDSHAKE_BUF_LEN*2);
    if (buf == NULL) {
        return RTMP_FAILED;
    }

    r->buf = buf;
    r->last = r->buf;
    r->end = r->buf + HANDSHAKE_BUF_LEN;

    w->buf = r->end;
    w->last = w->buf;
    w->end = w->buf + HANDSHAKE_BUF_LEN;

    return RTMP_OK;
}

static int32_t rtmp_handshake_prepare_send_c0c1(rtmp_session_t *session)
{
    return RTMP_OK;
}

static int32_t rtmp_handshake_prepare_recv_s0s1(rtmp_session_t *session)
{
    return RTMP_OK;
}

static int32_t rtmp_handshake_prepare_recv_s2(rtmp_session_t *session)
{
    return RTMP_OK;
}

static int32_t rtmp_handshake_prepare_send_c2(rtmp_session_t *session)
{
    return RTMP_OK;
}


static void rtmp_handshake_prepare_recv_c0c1(rtmp_session_t *session)
{
    mem_buf_t           *r,*w;
    rtmp_handshake_t    *h;

    h = & session->handshake;

    r = & h->rbuf;
    w = & h->wbuf;

    r->end = r->buf + HANDSHAKE_BUF_LEN;
    w->end = w->buf;
}

static int32_t rtmp_handshake_prepare_send_s0s1(rtmp_session_t *session)
{
    uint8_t             *c0,*c1,*s0,*s1;
    rtmp_handshake_t    *handshake;

    handshake = & session->handshake;

    c0 = handshake->rbuf.buf;
    c1 = c0 + 1;
    s0 = handshake->wbuf.buf;
    s1 = s0 + 1;

    if (*c0 != 0x03) {
        return RTMP_FAILED;
    }

    if (*(int32_t *)(c1 + 4) == 0) {
        handshake->old = 1;
    }

    handshake->c1s1_time = rtmp_current_sec;
    byte_read_4((char *)c1,(char *)&handshake->peer_epoch);
    
    *s0 = 0x03;

    byte_write_4((char*)&rtmp_current_sec,(char*)s1);
    memcpy(s1 + 4, rtmp_server_version, 4);
    byte_fill_random((char*)s1+8,1528);

    if (handshake->old == 0) {
        handshake_calc_c1s1_digest(s1,server_public_key,36);
    }

    handshake->wbuf.end = s0 + HANDSHAKE_BUF_LEN;

    return RTMP_OK;
}

static int32_t rtmp_handshake_prepare_send_s2(rtmp_session_t *session)
{
    uint8_t             *c0,*c1,*s2;
    rtmp_handshake_t    *handshake;

    handshake = & session->handshake;

    c0 = handshake->rbuf.buf;
    c1 = c0 + 1;
    s2 = handshake->wbuf.buf + 1;

    memcpy(s2, c1, 1536);

    if (handshake->old == 0) {

        uint8_t     digest[32],*d,*p[4];
        uint32_t    x;

        byte_write_4((char*)&handshake->c1s1_time,(char*)s2+4);

        x = handshake_get_x(c1 + 776);
        d = c1 + 776 + x;

        memcpy(digest,d,32);
        handshake_calc_c1s1_digest(c1,client_public_key,30);

        if (memcmp(d,digest,32) != 0) {
            return RTMP_FAILED;
        }

        handshake->digest = d;

        p[0] = handshake->digest;
        p[1] = p[0] + 32;
        p[2] = p[3] = p[1];

        /*create digest key*/
        handshake_make_digest(server_public_key,68,digest,p);

        
        p[0] = s2;
        p[1] = s2 + 1504;
        p[2] = p[3] = p[1];

        /*create s2*/
        handshake_make_digest(digest,32,p[1],p);
    }

    handshake->wbuf.last = s2;
    handshake->wbuf.end = s2 + HANDSHAKE_BUF_LEN-1;

    return RTMP_OK;
}

static int32_t rtmp_handshake_prepare_recv_c2(rtmp_session_t *session)
{
    return RTMP_OK;
}

static int32_t rtmp_handshake_verify_s2(rtmp_session_t *session)
{
    return RTMP_OK;
}

static int32_t rtmp_handshake_verify_c2(rtmp_session_t *session)
{
    return RTMP_OK;
}

static void handshake_calc_c1s1_digest(uint8_t *c1s1,uint8_t *key,int len)
{
    uint8_t *p[4];
    uint32_t x;

    x = handshake_get_x(c1s1 + 776);

    p[0] = c1s1;
    p[1] = p[0] + 776 + x;
    p[2] = p[0] + 808 + x;
    p[3] = p[0] + 1536;

    handshake_make_digest(key,len,p[1],p);
}