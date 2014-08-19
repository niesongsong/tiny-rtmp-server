
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

static rtmp_msg_handler_t rtmp_msg_handler[RTMP_MSG_MAX] = {
    {0,                     rtmp_handler_null},
    {RTMP_MSG_CHUNK_SIZE,   rtmp_handler_chunksize},    /*1*/
    {RTMP_MSG_ABORT,        rtmp_handler_abort},        /*2*/
    {RTMP_MSG_ACK,          rtmp_handler_ack},          /*3*/
    {RTMP_MSG_USER,         rtmp_handler_user},         /*4*/
    {RTMP_MSG_ACK_SIZE,     rtmp_handler_acksize},      /*5*/
    {RTMP_MSG_BANDWIDTH,    rtmp_handler_bandwidth},    /*6*/
    {RTMP_MSG_EDGE,         rtmp_handler_edge},         /*7*/
    {RTMP_MSG_AUDIO,        rtmp_handler_avdata},         /*8*/
    {RTMP_MSG_VIDEO,        rtmp_handler_avdata},         /*9*/
    {10,                    rtmp_handler_null},         /*10*/
    {11,                    rtmp_handler_null},         /*11*/
    {12,                    rtmp_handler_null},         /*12*/
    {13,                    rtmp_handler_null},         /*13*/
    {14,                    rtmp_handler_null},         /*14*/
    {RTMP_MSG_AMF3_META,    rtmp_handler_amf3meta},     /*15*/
    {RTMP_MSG_AMF3_SHARED,  rtmp_handler_amf3shared},   /*16*/
    {RTMP_MSG_AMF3_CMD,     rtmp_handler_amf3cmd},      /*17*/
    {RTMP_MSG_AMF_META,     rtmp_handler_amfmeta},      /*18*/
    {RTMP_MSG_AMF_SHARED,   rtmp_handler_amfshared},    /*19*/
    {RTMP_MSG_AMF_CMD,      rtmp_handler_amfcmd},       /*20*/
    {21,                    rtmp_handler_null},         /*21*/
    {RTMP_MSG_AGGREGATE,    rtmp_handler_aggregate},    /*22*/
};

/*RTMP_MSG_AMF_META*/
static rtmp_msg_amf_handler_t rtmp_msg_cmd_handler[] = {
    {"connect",         rtmp_amf_cmd_connect},
    {"createStream",    rtmp_amf_cmd_createstream},
    {"closeStream",     rtmp_amf_cmd_closestream},
    {"deleteStream",    rtmp_amf_cmd_deletestream},
    {"releaseStream",   rtmp_amf_cmd_releasestream},
    {"publish",         rtmp_amf_cmd_publish},
    {"publish",         rtmp_amf_cmd_publish},
    {"publish",         rtmp_amf_cmd_publish},
    {"play",            rtmp_amf_cmd_play},
    {"play2",           rtmp_amf_cmd_play2},
    {"seek",            rtmp_amf_cmd_seek},
    {"pause",           rtmp_amf_cmd_pause},
    {"pauseraw",        rtmp_amf_cmd_pause},
};

static void* rtmp_amf_pmalloc(size_t size,void *u);
static void  rtmp_amf_pfree(void *p,void *u);
static mem_buf_t* rtmp_copy_chains_to_temp_buf(rtmp_chunk_stream_t *st,
    mem_pool_t *temp_pool);

int32_t rtmp_handler_init(rtmp_cycle_t *cycle)
{
    rtmp_msg_handler_t **handler;
    int32_t              i;
    

    if (array_init(&cycle->msg_handler,cycle->pool,RTMP_MSG_MAX,
        sizeof(rtmp_msg_handler_t*)) != RTMP_OK) 
    {
        return RTMP_FAILED;
    }

    handler = array_push_n(&cycle->msg_handler,RTMP_MSG_MAX);
    if (handler == NULL) {
        return RTMP_FAILED;
    }

    for (i = 0;i < RTMP_MSG_MAX;i++) {
        handler[i] = &rtmp_msg_handler[i];
    }

    return RTMP_OK;
}

int32_t rtmp_handler_null(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}

int32_t rtmp_handler_chunksize(rtmp_session_t *session,
    rtmp_chunk_stream_t *st)
{
    mem_buf_t           *buf;
    uint32_t             chunksize;

    mem_reset_pool(session->temp_pool);
    buf = rtmp_copy_chains_to_temp_buf(st,session->temp_pool);
    if (buf == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]copy chain failed!",session->sid);
        return RTMP_FAILED;
    }

    if (buf->last - buf->buf < 4) {
        rtmp_log(RTMP_LOG_ERR,"[%d]invalid set chunk message!",
            session->sid);
        return RTMP_FAILED;
    }

    chunksize = ((uint32_t)(buf->buf[0] & 0x7f) << 24) 
              + ((uint32_t)buf->buf[1] << 16)
              + ((uint32_t)buf->buf[2] << 8) 
              + ((uint32_t)buf->buf[3]);

    session->in_chunk_size = chunksize;

    return RTMP_OK;
}

int32_t rtmp_handler_abort(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}
int32_t rtmp_handler_ack(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}

int32_t rtmp_handler_user(rtmp_session_t *session,rtmp_chunk_stream_t *st)
{
    mem_buf_t           *buf;
    uint16_t             evt;

    mem_reset_pool(session->temp_pool);
    buf = rtmp_copy_chains_to_temp_buf(st,session->temp_pool);
    if (buf == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]copy chain failed!",session->sid);
        return RTMP_FAILED;
    }

    if (buf->last - buf->buf < 2) {
        rtmp_log(RTMP_LOG_ERR,"[%d]invalid user event!",session->sid);
        return RTMP_FAILED;
    }

    evt = ((uint16_t)buf->buf[0] << 8) + (uint16_t)buf->buf[1];
    switch (evt) {

    case RTMP_USER_STREAM_BEGIN:
        break;

    case RTMP_USER_STREAM_EOF:

        break;
    case RTMP_USER_STREAM_DRY:

        break;
    case RTMP_USER_SET_BUFLEN:

        break;
    case RTMP_USER_RECORDED:

        break;
    case RTMP_USER_PING_REQUEST:
        rtmp_send_ping_response(session,st,buf);

        break;

    case RTMP_USER_PING_RESPONSE:
        session->ping_flag = 0;
        break;

    default:
        break;
    }

    return RTMP_OK;
}

static mem_buf_t* rtmp_copy_chains_to_temp_buf(rtmp_chunk_stream_t *st,
    mem_pool_t *temp_pool)
{
    mem_buf_t           *buf;
    mem_buf_chain_t     *chain;
    uint8_t             *body;
    rtmp_chunk_header_t  hdr;
    int32_t              ncopy;

    buf = mem_palloc(temp_pool,sizeof(mem_buf_t));
    if (buf == NULL) {
        return NULL;
    }

    if (st->chain == st->chain->next) {
        body = rtmp_chunk_read(&st->chain->chunk,&hdr);
        if (body == NULL) {
            return NULL;
        }

        *buf = st->chain->chunk;
        buf->last = body;

    } else {

        buf->buf = mem_palloc(temp_pool,st->recvlen);
        if (buf->buf == NULL) {
            return NULL;
        }

        buf->last = buf->buf;
        buf->end = buf->buf + st->recvlen;
        ncopy = 0;

        chain = st->chain;
        while (chain) {
            body = rtmp_chunk_read(&chain->chunk,&hdr);
            if (body == NULL) {
                return NULL;
            }
            ncopy = chain->chunk.last - body;
            if (ncopy > 0) {
                memcpy(buf->last,body,ncopy);
                buf->last += ncopy;
            }
            chain = chain->next;
        }
    }

    return buf;
}

int32_t rtmp_handler_acksize(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}
int32_t rtmp_handler_bandwidth(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}
int32_t rtmp_handler_edge(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}

int32_t rtmp_handler_amf3meta(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}
int32_t rtmp_handler_amf3shared(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}

int32_t rtmp_handler_amf0_amf3cmd(rtmp_session_t *session,
    rtmp_chunk_stream_t *st,int type)
{
    mem_buf_t           *buf;
    int32_t              buflen,i,n;
    amf_data_t          *amfcmd[10];
    char                *cmd;

    amf_init(rtmp_amf_pmalloc,rtmp_amf_pfree,session->temp_pool);

    mem_reset_pool(session->temp_pool);
    buf = rtmp_copy_chains_to_temp_buf(st,session->temp_pool);
    if (buf == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]copy chain failed!",session->sid);
        return RTMP_FAILED;
    }

    memset(amfcmd,0,sizeof(amfcmd));
    if (type) {
        buf->buf++;
    }

    /*decode*/
    buflen = buf->last - buf->buf;
    for (n = 0;(buflen > 0) && (n < (int32_t)rtmp_array_size(amfcmd));n++) {

        amfcmd[n] = amf_decode((char*)buf->last-buflen,&buflen);
        if (amfcmd[n] == NULL) {
            break;
        }

#if 1
        amf_dump_data(amfcmd[n]);
#endif

    }

    /*command name*/
    if (amfcmd[0]) {
        cmd = amf_get_string(amfcmd[0]);
        if (cmd == NULL) {
            return RTMP_FAILED;
        }

        for (i = 0;i < (int32_t)rtmp_array_size(rtmp_msg_cmd_handler);i++) {
            if (strcmp(cmd,rtmp_msg_cmd_handler[i].command) == 0) {
                if (rtmp_msg_cmd_handler[i].pt) {
                    return rtmp_msg_cmd_handler[i].pt(session,st,amfcmd,n);
                }
            }
        }
    }

    return RTMP_OK;
}

int32_t rtmp_handler_amf3cmd(rtmp_session_t *session,
    rtmp_chunk_stream_t *st)
{
    return rtmp_handler_amf0_amf3cmd(session,st,3);
}
int32_t rtmp_handler_amfmeta(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}
int32_t rtmp_handler_amfshared(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}

int32_t rtmp_handler_amfcmd(rtmp_session_t *session,
    rtmp_chunk_stream_t *st)
{
    return rtmp_handler_amf0_amf3cmd(session,st,0);
}

int32_t rtmp_handler_aggregate(rtmp_session_t *s,rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}

static void* rtmp_amf_pmalloc(size_t size,void *u)
{
    return mem_palloc((mem_pool_t *)(u),size);
}

static void rtmp_amf_pfree(void *p,void *u)
{
    return;
}

int32_t rtmp_amf_cmd_closestream(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    return RTMP_OK;
}

int32_t rtmp_amf_cmd_deletestream(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    return RTMP_OK;
}

int32_t rtmp_amf_cmd_play2(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    return RTMP_FAILED;
}

int32_t rtmp_amf_cmd_seek(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    return RTMP_FAILED;
}

int32_t rtmp_amf_cmd_pause(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    return RTMP_FAILED;
}

void rtmp_send_ping_response(rtmp_session_t *session,
    rtmp_chunk_stream_t *st,mem_buf_t *buf)
{
    uint32_t            timestamp;
    int32_t             rc;
    mem_buf_chain_t    *chain;

    timestamp = 0;

    timestamp += (uint32_t)buf->buf[2] << 24;
    timestamp += (uint32_t)buf->buf[3] << 16;
    timestamp += (uint32_t)buf->buf[4] << 8;
    timestamp += (uint32_t)buf->buf[5];


    chain = rtmp_create_ping_response(session,timestamp);

    if (chain == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]create ping response failed!",
            session->sid);
        return;
    }

    rc = rtmp_append_message_chain(session,chain);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_ERR,"[%d]append response message failed!",
            session->sid);
        return ;
    }

    rtmp_chain_send(session->c->write);
    return ;
}

void rtmp_send_ping_request(rtmp_session_t *session)
{
    uint32_t            timestamp;
    int32_t             rc;
    mem_buf_chain_t    *chain;

    timestamp = rtmp_current_sec;

    chain = rtmp_create_ping_request(session,timestamp);

    if (chain == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]create ping request failed!",
            session->sid);
        return;
    }

    rc = rtmp_append_message_chain(session,chain);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_ERR,"[%d]append request message failed!",
            session->sid);
        return ;
    }

    rtmp_chain_send(session->c->write);

    return ;
}