
/*
 * CopyLeft (C) nie950@gmail.com
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

static void handshake_calc_c1s1_digest(uint8_t *c1s1,uint8_t *key,int len,uint8_t style);

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
    int32_t             rc;

    conn = ev->data;
    session = conn->data;
    hs = session->handshake;

    if (ev->timeout) {
        rtmp_session_destroy(session);
        return;
    }

    if (ev->timer_set) {
        rtmp_event_del_timer(ev);
    }

    rc = rtmp_recv_buf(conn->fd, &hs->rbuf,NULL);
    if (rc == SOCK_EAGAIN) {
        if (!ev->active) {
            rtmp_event_add(ev,EVENT_READ);
        }
        rtmp_event_add_timer(ev,6000);
        return ;
    }

    if (rc == SOCK_ERROR) {
        rtmp_log(RTMP_LOG_DEBUG,"[%d] handshake failed:%d",session->sid,rc);
        rtmp_session_destroy(session);
        return;
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
        rtmp_log(RTMP_LOG_ERR,"unknown stage[%d]",hs->stage);
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
    int32_t             rc;

    conn = ev->data;
    session = conn->data;
    hs = session->handshake;

    if (ev->timeout) {
        rtmp_session_destroy(session);
        return;
    }

    if (ev->timer_set) {
        rtmp_event_del_timer(ev);
    }

    rc = rtmp_send_buf(conn->fd, &hs->wbuf,NULL);
    if (rc == SOCK_EAGAIN) {    
        rtmp_log(RTMP_LOG_DEBUG,"[%d] send error:%d",
            session->sid,sock_errno);
        if (!ev->active) {
            rtmp_event_add(ev,EVENT_WRITE);
        }
        rtmp_event_add_timer(ev,6000);
        return ;
    }

    if (rc == SOCK_ERROR) {
        rtmp_session_destroy(session);
        return;
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
        rtmp_log(RTMP_LOG_ERR,"unknown stage[%d]",hs->stage);
        rtmp_session_destroy(session);
        break;
    }

    return ;
}

rtmp_handshake_t* rtmp_handshake_alloc(mem_pool_t *pool)
{
    mem_buf_t        *r,*w;
    uint8_t          *buf;
    rtmp_handshake_t *handshake;

    handshake = mem_pcalloc(pool,
        sizeof(rtmp_handshake_t) + HANDSHAKE_BUF_LEN*2);
    if (handshake == NULL) {
        return NULL;
    }

    buf = (uint8_t *)handshake + sizeof(rtmp_handshake_t);
    r = &handshake->rbuf;
    w = &handshake->wbuf;

    if (buf == NULL) {
        return NULL;
    }

    r->buf = buf;
    r->last = r->buf;
    r->end = r->buf + HANDSHAKE_BUF_LEN;

    w->buf = r->end;
    w->last = w->buf;
    w->end = w->buf + HANDSHAKE_BUF_LEN;

    return handshake;
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

    h = session->handshake;

    r = & h->rbuf;
    w = & h->wbuf;

    r->end = r->buf + HANDSHAKE_BUF_LEN;
    w->end = w->buf;
}

static int32_t rtmp_handshake_prepare_send_s0s1(rtmp_session_t *session)
{
    uint8_t             *c0,*c1,*s0,*s1;
    rtmp_handshake_t    *handshake;

    handshake = session->handshake;

    c0 = handshake->rbuf.buf;
    c1 = c0 + 1;
    s0 = handshake->wbuf.buf;
    s1 = s0 + 1;

    if (*c0 != 0x03) {
        return RTMP_FAILED;
    }

    handshake->peer_epoch = byte_make_ulong4(c1);
    *s0 = 0x03;
    ulong_make_byte4(s1,rtmp_current_sec);
    memset(s1 + 4, 0, 4);
    byte_fill_random((char*)s1+8,1528);
    
    if (*(int32_t *)(c1 + 4) != 0) {
        memcpy(s1 + 4, rtmp_server_version, 4);
        handshake_calc_c1s1_digest(s1,server_public_key,36,0);
    }

    handshake->wbuf.end = s0 + HANDSHAKE_BUF_LEN;

    return RTMP_OK;
}

static int32_t rtmp_handshake_prepare_send_s2(rtmp_session_t *session)
{
    uint8_t             *c0,*c1,*s2;
    rtmp_handshake_t    *handshake;
    uint8_t              digest[32],*d,*p[4];
    uint32_t             x;

    handshake = session->handshake;

    c0 = handshake->rbuf.buf;
    c1 = c0 + 1;
    s2 = handshake->wbuf.buf + 1;

    memcpy(s2, c1, 1536);
    memset(s2 + 4,0,4);

    do {
        if (*(int32_t *)(c1 + 4) == 0) {
            break;
        }

        x = handshake_get_x(c1 + 8);
        d = c1 + 8 + x;

        memcpy(digest,d,32);
        handshake_calc_c1s1_digest(c1,client_public_key,30,0);

        if (memcmp(d,digest,32) == 0) {
            handshake->digest = d;
            goto found_digest;
        }
        memcpy(d,digest,32);
        
        x = handshake_get_x(c1 + 776);
        d = c1 + 776 + x;

        memcpy(digest,d,32);
        handshake_calc_c1s1_digest(c1,client_public_key,30,1);

        if (memcmp(d,digest,32) == 0) {
            handshake->digest = d;
            goto found_digest;
        }
        memcpy(d,digest,32);

found_digest:
        p[0] = handshake->digest;
        p[1] = p[0] + 32;
        p[2] = p[3] = p[1];

        /*create digest key*/
        handshake_make_digest(server_public_key,68,digest,p);

        byte_fill_random((char*)s2,1504);

        p[0] = s2;
        p[1] = p[0] + 1504;
        p[2] = p[3] = p[1];

        handshake_make_digest(digest,32,p[1],p);
    } while (0);

    handshake->wbuf.last = s2;
    handshake->wbuf.end = s2 + HANDSHAKE_BUF_LEN-1;

    return RTMP_OK; 
}

static int32_t rtmp_handshake_prepare_recv_c2(rtmp_session_t *session)
{
    mem_buf_t           *r,*w;
    rtmp_handshake_t    *h;

    h = session->handshake;

    r = & h->rbuf;
    w = & h->wbuf;

    r->end  = r->buf + HANDSHAKE_BUF_LEN;
    r->last = r->buf + 1;

    w->end = w->buf;

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

static void handshake_calc_c1s1_digest(
    uint8_t *c1s1,uint8_t *key,int len,uint8_t style)
{
    uint8_t *p[4];
    uint32_t x,offset;


    offset = style ? 8:776;
    x = handshake_get_x(c1s1 + offset);

    p[0] = c1s1;
    p[1] = p[0] + offset + x;
    p[2] = p[0] + offset + x + 32;
    p[3] = p[0] + 1536;

    handshake_make_digest(key,len,p[1],p);
}