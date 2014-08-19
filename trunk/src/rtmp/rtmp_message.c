
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

#define RTMP_PROTO_HEAD_LEN   12

static mem_buf_chain_t *
rtmp_create_proctol_message(rtmp_session_t *session,uint8_t type,uint8_t len);

static mem_buf_chain_t *
rtmp_create_status_message(rtmp_session_t *session,rtmp_chunk_header_t *h,
    char *code, char* level,char *desc);

/*
 * 5.4. Protocol Control Messages
 *
 *  using message type IDs          1, 2, 3, 4, 5, and 6 
 *  using message stream ID         0
 *  using chunk stream ID           2
 *  their timestamps are ignored    0
 *
 *  RTMP_MSG_CHUNK_SIZE         1
 *  RTMP_MSG_ABORT              2
 *  RTMP_MSG_ACK                3
 *  RTMP_MSG_USER               4
 *  RTMP_MSG_ACK_SIZE           5
 *  RTMP_MSG_BANDWIDTH          6
 *
 */

static mem_buf_chain_t *
rtmp_create_proctol_message(rtmp_session_t *session,uint8_t type,uint8_t len)
{
    mem_buf_chain_t *chain;
    uint8_t         *head;

    if (type < RTMP_MSG_CHUNK_SIZE || type > RTMP_MSG_BANDWIDTH) {
        return NULL;
    }

    if (session->out_chunk_size < len) {
        rtmp_log(RTMP_LOG_ERR,"too large len:[%d] chunk size:[%d]",
            len,session->out_chunk_size);
        return NULL;
    }

    chain = rtmp_core_alloc_chain(session,session->chunk_pool,
        session->out_chunk_size);
    if (chain == NULL) {
        return NULL;
    }

    head = chain->chunk.buf + RTMP_MAX_BASICHEADER - RTMP_PROTO_HEAD_LEN;

    chain->chunk.last = head + RTMP_PROTO_HEAD_LEN;
    chain->chunk.end = chain->chunk.last + len;
    
    memset(head,0,RTMP_PROTO_HEAD_LEN);

    /*fmt = 0, csid = 2*/
    head[0] = 0x02; 
    
    /*head[1] ~ head[3]: timestamp ignored*/
    
    /*head[4] ~ head[6]: len*/
    head[6] = len;

    /*head[7]: type id*/
    head[7] = type;

    /*head[8] ~ head[11]: message id = 0*/
    
    return chain;
}

mem_buf_chain_t* 
rtmp_create_set_chunk_size(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_message(session,RTMP_MSG_CHUNK_SIZE,4);
    if (chain) {

        byte_write_int32((const char*)&session->out_chunk_size,
            (char*)chain->chunk.last);

        chain->chunk.last -= RTMP_PROTO_HEAD_LEN;
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_window_ack_window(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_message(session,RTMP_MSG_ACK_SIZE,4);
    if (chain) {
        byte_write_int32((const char*)&session->ack_window,
            (char*)chain->chunk.last);
        chain->chunk.last -= RTMP_PROTO_HEAD_LEN;
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_peer_bandwidth_size(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_message(session,RTMP_MSG_BANDWIDTH,5);
    if (chain) {
        byte_write_int32((const char*)&session->ack_window,
            (char*)chain->chunk.last);

        chain->chunk.last[4] = 0x02;
        chain->chunk.last -= RTMP_PROTO_HEAD_LEN;
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_user_begin(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_message(session,RTMP_MSG_USER,6);
    if (chain) {
        memset(chain->chunk.last,0,6);
        chain->chunk.last[5] = 0x01;
        chain->chunk.last -= RTMP_PROTO_HEAD_LEN;
    }

    return chain;
}

mem_buf_chain_t* rtmp_prepare_memssage_chain(rtmp_session_t *session,
    rtmp_chunk_header_t *sthead,mem_buf_chain_t *in_chain)
{
    mem_buf_chain_t     *chain,*head,*last,*next;
    mem_buf_t            src,*dest,buf;
    uint32_t             n;
    rtmp_chunk_header_t  hdr;
    uint8_t              headbuf[RTMP_MAX_BASICHEADER],*body,*end;

    hdr = *sthead;

    buf.buf = headbuf;
    buf.end = headbuf + sizeof(headbuf);

    head = last = next = NULL;
    dest = NULL;
    end = NULL;

    for (chain = in_chain;chain; chain= chain->next) {

        src = chain->chunk;
        src.buf = rtmp_chunk_read(&src,&hdr);

        if (src.buf == NULL) {
            rtmp_core_free_chains(session,session->chunk_pool,head);
            rtmp_log(RTMP_LOG_ERR,"[%d]error chunk!",session->sid);

            return NULL;
        }

        while (src.buf != src.last) {
            n = src.last - src.buf;

            if (dest == NULL) {

                last = next;
                next = rtmp_core_alloc_chain(session,
                    session->chunk_pool,session->out_chunk_size);

                if (next == NULL) {
                    rtmp_core_free_chains(session,session->chunk_pool,head);
                    rtmp_log(RTMP_LOG_ERR,"[%d]alloc chain failed!",
                        session->sid);

                    return NULL;
                }

                next->next = NULL;
                dest = & next->chunk;

                if (head == NULL) {
                    head = next;
                }

                if (last != NULL) {
                    last->next = next;
                }

                buf.last = headbuf;
                rtmp_chunk_write(&buf,&hdr);

                body = dest->buf + RTMP_MAX_BASICHEADER;
                dest->last = body - (buf.last - buf.buf);

                memcpy(dest->last,buf.buf,buf.last - buf.buf);
                end = dest->end;

                dest->end = body;
                hdr.fmt = 3;  /*fragment*/
            }

            if (n >= (uint32_t)(end - dest->end)) {
                n = end - dest->end;
            }

            memcpy(dest->end,src.buf,n);            
            dest->end += n;
            src.buf += n;

            if (end == dest->end) {
                dest = NULL;
            }
        }
    }

    return head;
}

mem_buf_chain_t* rtmp_prepare_memssage_buf(rtmp_session_t *session,
    rtmp_chunk_header_t *sthead,mem_buf_t *message)
{
    int32_t              mlen,nchains,n,m;
    mem_buf_chain_t     *chain,*next;
    rtmp_chunk_header_t  hdr;
    mem_buf_t            buf;
    uint8_t              headbuf[RTMP_MAX_BASICHEADER],*body;
    mem_pool_t          *chunk_pool;


    mlen = message->last - message->buf;
    nchains = (mlen + session->out_chunk_size - 1)/session->out_chunk_size;
    
    hdr = *sthead;
    hdr.msglen = mlen;

    buf.buf = headbuf;
    buf.end = headbuf + sizeof(headbuf);

    chunk_pool = session->chunk_pool;
    chain = NULL;
    for (next = NULL,n = nchains; n > 0; n--) {

        buf.last = headbuf;
        if (rtmp_chunk_write(&buf,&hdr) == -1) {
            rtmp_log(RTMP_LOG_ERR,"[%d] write basic header failed!",session->sid);

            if (next) {
                rtmp_core_free_chains(session,chunk_pool,next);
            }

            break;
        }

        chain = rtmp_core_alloc_chain(session,chunk_pool,session->out_chunk_size);

        if (chain == NULL) {
            if (next) {
                rtmp_core_free_chains(session,chunk_pool,next);
            }
            break;
        }

        body = chain->chunk.buf + RTMP_MAX_BASICHEADER;

        /*copy head*/
        chain->chunk.last = body - (buf.last - buf.buf);
        memcpy(chain->chunk.last,buf.buf,buf.last - buf.buf);

        /*copy message*/
        m = rtmp_min(mlen,(int32_t)session->out_chunk_size);
        memcpy(body,message->last - mlen,m);
        chain->chunk.end = body + m;
        mlen -= m;

        chain->next = next;
        next = chain;

        hdr.fmt = 3;
    }

    return chain;
}

int32_t rtmp_append_message_chain(rtmp_session_t *session,
    mem_buf_chain_t *chain)
{
    uint32_t    front;

    front = session->out_front;

    front = (front + 1) % session->out_queue;
    if (front == session->out_rear) {
        rtmp_log(RTMP_LOG_WARNING,"message queue full!");
        return RTMP_FAILED;
    }

    session->out_message[session->out_front] = chain;
    session->out_front = front;

    return RTMP_OK;
}

int32_t rtmp_create_append_chain(rtmp_session_t *session,
    rtmp_create_proctol_message_ptr ptr,rtmp_chunk_header_t *h)
{
    mem_buf_chain_t *chain;

    chain = ptr(session,h);
    if (chain == NULL) {
        return RTMP_FAILED;
    }

    if (rtmp_append_message_chain(session,chain) == -1) {
        rtmp_core_free_chains(session,session->chunk_pool,chain);
        return RTMP_FAILED;
    }

    return RTMP_OK;
}

mem_buf_chain_t* 
rtmp_create_play_reset(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    return rtmp_create_status_message(session,h,
        "NetStream.Play.Reset","status","Playing and resetting .");
}

mem_buf_chain_t* 
rtmp_create_play_start(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    return rtmp_create_status_message(session,h,
        "NetStream.Play.Start","status","Start live");
}

mem_buf_chain_t* 
rtmp_create_publish_badname(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    return rtmp_create_status_message(session,h,
        "NetStream.Publish.BadName","error","Already publishing");
}

mem_buf_chain_t* 
rtmp_create_publish_start(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    return rtmp_create_status_message(session,h,
        "NetStream.Publish.Start","status","Start publishing");
}

mem_buf_chain_t* 
rtmp_create_play_not_found(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    return rtmp_create_status_message(session,h,
        "NetStream.Play.StreamNotFound","error","No such stream");
}

mem_buf_chain_t* 
rtmp_create_publish_not_found(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    return rtmp_create_status_message(session,h,
        "NetStream.Publish.StreamNotFound","error","No such stream");
}


mem_buf_chain_t* 
rtmp_create_sample_access(rtmp_session_t *session,rtmp_chunk_header_t *h)
{
    amf_data_t              *amf[3];
    mem_buf_t               *buf;
    mem_buf_chain_t         *chain;
    rtmp_chunk_header_t      hdr;

    amf[0] = amf_new_string("|RtmpSampleAccess",0);
    amf[1] = amf_new_bool(0);
    amf[2] = amf_new_bool(0);

    buf = rtmp_prepare_amf_buffer(session->temp_pool,amf,3);
    if (buf == NULL) {
        return NULL;
    }

    memset(&hdr,0,sizeof(hdr));

    hdr.fmt = 0;
    hdr.csid = 4;

    hdr.msglen = buf->last - buf->buf;
    hdr.msgtid = RTMP_MSG_AMF_META;
    hdr.msgsid = 1;

    chain = rtmp_prepare_memssage_buf(session,&hdr,buf);

    if (chain == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]prepare message failed!",
            session->sid);

    }
    return chain;
}

mem_buf_chain_t*
rtmp_create_status_message(rtmp_session_t *session,rtmp_chunk_header_t *h,
    char *code, char* level,char *desc)
{
    amf_data_t              *amf[4];
    mem_buf_t               *buf;
    mem_buf_chain_t         *chain;
    rtmp_chunk_header_t      hdr;

    amf[0] = amf_new_string("onStatus",0);
    amf[1] = amf_new_number(0.0);
    amf[2] = amf_new_null();
    amf[3] = amf_new_object();

    amf_put_prop(amf[3],"level",amf_new_string(level,0));
    amf_put_prop(amf[3],"code",amf_new_string(code,0));
    amf_put_prop(amf[3],"description",amf_new_string(desc,0));

    buf = rtmp_prepare_amf_buffer(session->temp_pool,amf,4);
    if (buf == NULL) {
        return NULL;
    }

    memset(&hdr,0,sizeof(hdr));

    hdr.fmt = 0;
    hdr.csid = 4;

    hdr.msglen = buf->last - buf->buf;
    hdr.msgtid = RTMP_MSG_AMF_CMD;
    hdr.msgsid = h->msgsid;

    chain = rtmp_prepare_memssage_buf(session,&hdr,buf);

    if (chain == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]prepare message failed!",
            session->sid);

    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_ping_request(rtmp_session_t *session,uint32_t timestamp)
{
    mem_buf_chain_t *chain;
    uint16_t         type;

    chain = rtmp_create_proctol_message(session,RTMP_MSG_USER,6);
    if (chain) {
        type = RTMP_USER_PING_REQUEST;
        
        chain->chunk.last[0] = (uint8_t)((type & 0xff00) >> 8);
        chain->chunk.last[1] = (uint8_t)(type & 0x00ff);
        
        chain->chunk.last[2] = (uint8_t)(timestamp & 0xff000000 >> 24);
        chain->chunk.last[3] = (uint8_t)(timestamp & 0x0000ff00 >> 16);
        chain->chunk.last[4] = (uint8_t)(timestamp & 0x00ff0000 >> 8);
        chain->chunk.last[5] = (uint8_t)(timestamp & 0x000000ff);

        chain->chunk.last -= RTMP_PROTO_HEAD_LEN;
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_ping_response(rtmp_session_t *session,uint32_t timestamp)
{
    mem_buf_chain_t *chain;
    uint16_t         type;

    chain = rtmp_create_proctol_message(session,RTMP_MSG_USER,6);
    if (chain) {
        type = RTMP_USER_PING_RESPONSE;

        chain->chunk.last[0] = (uint8_t)((type & 0xff00) >> 8);
        chain->chunk.last[1] = (uint8_t)(type & 0x00ff);

        chain->chunk.last[2] = (uint8_t)(timestamp & 0xff000000 >> 24);
        chain->chunk.last[3] = (uint8_t)(timestamp & 0x0000ff00 >> 16);
        chain->chunk.last[4] = (uint8_t)(timestamp & 0x00ff0000 >> 8);
        chain->chunk.last[5] = (uint8_t)(timestamp & 0x000000ff);

        chain->chunk.last -= RTMP_PROTO_HEAD_LEN;
    }

    return chain;
}