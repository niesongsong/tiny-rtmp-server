
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

#define RTMP_PROTO_HEAD_LEN   12

static mem_buf_chain_t *
rtmp_create_status_chain(rtmp_session_t *session,rtmp_chunk_header_t *h,
    char *code, char* level,char *desc,rtmp_chunk_header_t *hdr);

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

static 
mem_buf_chain_t * rtmp_create_proctol_chain(rtmp_session_t *session,
    uint8_t type,uint8_t len,rtmp_chunk_header_t *hdr)
{
    mem_buf_chain_t *chain;

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
    chain->chunk.last = chain->chunk.buf + RTMP_MAX_CHUNK_HEADER;
    chain->chunk.end = chain->chunk.last + len;

    memset(hdr,0,sizeof(rtmp_chunk_header_t));

    hdr->fmt = 0;
    hdr->csid = 2;
    hdr->msglen = len;
    hdr->msgtid = type;

    return chain;
}

mem_buf_chain_t* 
rtmp_create_set_chunk_size(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_chain(session,RTMP_MSG_CHUNK_SIZE,4,hdr);
    if (chain) {
        ulong_make_byte4(chain->chunk.last,session->out_chunk_size);
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_ack_size(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_chain(session,RTMP_MSG_ACK_SIZE,4,hdr);
    if (chain) {
        ulong_make_byte4(chain->chunk.last,session->ack_window);
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_ack(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_chain(session,RTMP_MSG_ACK,4,hdr);
    if (chain) {
        ulong_make_byte4(chain->chunk.last,session->in_bytes);
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_peer_bandwidth_size(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_chain(session,RTMP_MSG_BANDWIDTH,5,hdr);
    if (chain) {
        ulong_make_byte4(chain->chunk.last,session->ack_window);
        chain->chunk.last[4] = 0x02;
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_user_begin(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    mem_buf_chain_t *chain;

    chain = rtmp_create_proctol_chain(session,RTMP_MSG_USER,6,hdr);
    if (chain) {
        memset(chain->chunk.last,0,6);
        chain->chunk.last[5] = 0x01;
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
    uint8_t              headbuf[RTMP_MAX_CHUNK_HEADER],*body,*end;

    hdr = *sthead;

    buf.buf = headbuf;
    buf.end = headbuf + sizeof(headbuf);

    head = last = next = NULL;
    dest = NULL;
    end = NULL;

    for (chain = in_chain;chain; chain= chain->next) {
        src = chain->chunk;

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

                body = dest->buf + RTMP_MAX_CHUNK_HEADER;
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

mem_buf_t* rtmp_copy_chain_to_buf(mem_buf_chain_t *chain_in,
    mem_pool_t *temp_pool)
{
    mem_buf_t           *buf;
    mem_buf_chain_t     *chain;
    int32_t              ncopy;

    buf = mem_palloc(temp_pool,sizeof(mem_buf_t));
    if (buf == NULL) {
        return NULL;
    }

    if (chain_in->next == NULL) {

        *buf = chain_in->chunk;

    } else {
        ncopy = 0;
        chain = chain_in;
        while (chain) {
            ncopy += chain->chunk.last - chain->chunk.buf;
            chain = chain->next;
        }

        buf->buf = mem_palloc(temp_pool,ncopy);
        if (buf->buf == NULL) {
            return NULL;
        }

        buf->last = buf->buf;
        buf->end = buf->buf + ncopy;
        ncopy = 0;

        chain = chain_in;
        while (chain) {
            ncopy = chain->chunk.last - chain->chunk.buf;
            if (ncopy > 0) {
                memcpy(buf->last,chain->chunk.buf,ncopy);
                buf->last += ncopy;
            }
            chain = chain->next;
        }
    }

    return buf;
}

mem_buf_chain_t *rtmp_copy_buf_to_chain(rtmp_session_t *session,
    mem_buf_t *buf)
{
    int32_t              mlen,nchains,n,m;
    mem_pool_t          *chunk_pool;
    mem_buf_chain_t     *chain,*next;
    uint8_t             *body;

    mlen = buf->last - buf->buf;
    nchains = (mlen + session->out_chunk_size - 1)/session->out_chunk_size;

    chunk_pool = session->chunk_pool;
    chain = NULL;
    for (next = NULL,n = nchains; n > 0; n--) {

        chain = rtmp_core_alloc_chain(session,chunk_pool,session->out_chunk_size);

        if (chain == NULL) {
            if (next) {
                rtmp_core_free_chains(session,chunk_pool,next);
            }
            break;
        }

        body = chain->chunk.buf + RTMP_MAX_CHUNK_HEADER;

        /*copy message to chain*/
        m = rtmp_min(mlen,(int32_t)session->out_chunk_size);
        memcpy(body,buf->last - mlen,m);

        chain->chunk.end = body + m;
        chain->chunk.last = body;
        mlen -= m;

        chain->next = next;
        next = chain;
    }

    return chain;
}

mem_buf_chain_t *rtmp_copy_chain_to_chain(rtmp_session_t *session,
    mem_buf_chain_t *in_chain)
{
    mem_buf_t       *buf;

    buf = rtmp_copy_chain_to_buf(in_chain,session->temp_pool);
    if (buf == NULL) {
        return NULL;
    }

    return rtmp_copy_buf_to_chain(session,buf);
}

int32_t rtmp_append_message_chain(rtmp_session_t *session,
    mem_buf_chain_t *chain,rtmp_chunk_header_t *hdr)
{
    uint32_t            front;
    rtmp_message_t     *msg;
    mem_buf_chain_t    *next;
    mem_buf_t           head;
    uint8_t             headbuf[RTMP_MAX_CHUNK_HEADER];
    rtmp_chunk_header_t fh; /*fragment header*/
    
    front = session->out_front;

    front = (front + 1) % session->out_queue;
    if (front == session->out_rear) {
        rtmp_log(RTMP_LOG_WARNING,"message queue full, droped!");
        return RTMP_FAILED;
    }

    fh = *hdr;

    fh.fmt = 3;

    head.buf  = headbuf;
    head.last = headbuf;
    head.end  = headbuf + sizeof (headbuf);

    if (rtmp_chunk_write(&head,hdr) == -1) {
        return RTMP_FAILED;
    }

    msg = &session->out_message[session->out_front];

    /*prepare chunk head*/
    msg->head.last -= head.last - head.buf;
    memcpy(msg->head.last,head.buf,head.last - head.buf);
    session->out_front = front;

    next = chain->next;
    while (next) {
        
        head.buf  = headbuf;
        head.last = headbuf;
        head.end  = headbuf + sizeof (headbuf);

        if (rtmp_chunk_write(&head,&fh) == -1) {
            return RTMP_FAILED;
        }

        chain->chunk.last -= head.last - head.buf;
        memcpy(chain->chunk.last,head.buf,head.last - head.buf);

        next = next->next;
    }

    msg->chain = chain;
    msg->hdr = *hdr;

    return RTMP_OK;
}

int32_t rtmp_create_append_chain(rtmp_session_t *session,
    rtmp_create_proctol_message_ptr ptr,
    rtmp_chunk_header_t *h)
{
    mem_buf_chain_t     *chain;
    rtmp_chunk_header_t  nh;

    chain = ptr(session,h,&nh);
    if (chain == NULL) {
        return RTMP_FAILED;
    }

    if (rtmp_append_message_chain(session,chain,&nh) == -1) {
        rtmp_core_free_chains(session,session->chunk_pool,chain);
        return RTMP_FAILED;
    }

    return RTMP_OK;
}

mem_buf_chain_t* 
rtmp_create_play_reset(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    return rtmp_create_status_chain(session,h,
        "NetStream.Play.Reset","status","Playing and resetting .",hdr);
}

mem_buf_chain_t* 
rtmp_create_play_start(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    return rtmp_create_status_chain(session,h,
        "NetStream.Play.Start","status","Start live",hdr);
}

mem_buf_chain_t* 
rtmp_create_publish_badname(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    return rtmp_create_status_chain(session,h,
        "NetStream.Publish.BadName","error","Already publishing",hdr);
}

mem_buf_chain_t* 
rtmp_create_publish_start(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    return rtmp_create_status_chain(session,h,
        "NetStream.Publish.Start","status","Start publishing",hdr);
}

mem_buf_chain_t* 
rtmp_create_play_not_found(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    return rtmp_create_status_chain(session,h,
        "NetStream.Play.StreamNotFound","error","No such stream",hdr);
}

mem_buf_chain_t* 
rtmp_create_publish_not_found(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    return rtmp_create_status_chain(session,h,
        "NetStream.Publish.StreamNotFound","error","No such stream",hdr);
}


mem_buf_chain_t* 
rtmp_create_sample_access(rtmp_session_t *session,rtmp_chunk_header_t *h,
    rtmp_chunk_header_t *hdr)
{
    amf_data_t              *amf[3];
    mem_buf_t               *buf;
    mem_buf_chain_t         *chain;

    amf[0] = amf_new_string("|RtmpSampleAccess",0);
    amf[1] = amf_new_bool(0);
    amf[2] = amf_new_bool(0);

    buf = rtmp_prepare_amf_buffer(session->temp_pool,amf,3);
    if (buf == NULL) {
        return NULL;
    }

    memset(hdr,0,sizeof(rtmp_chunk_header_t));

    hdr->fmt = 0;
    hdr->csid = 4;

    hdr->msglen = buf->last - buf->buf;
    hdr->msgtid = RTMP_MSG_AMF_META;
    hdr->msgsid = 1;

    chain = rtmp_copy_buf_to_chain(session,buf);

    if (chain == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]prepare message failed!",
            session->sid);

    }
    return chain;
}

mem_buf_chain_t*
rtmp_create_status_chain(rtmp_session_t *session,rtmp_chunk_header_t *h,
    char *code, char* level,char *desc,rtmp_chunk_header_t *hdr)
{
    amf_data_t              *amf[4];
    mem_buf_t               *buf;
    mem_buf_chain_t         *chain;

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

    memset(hdr,0,sizeof(rtmp_chunk_header_t));

    hdr->fmt = 0;
    hdr->csid = 4;
    hdr->msglen = buf->last - buf->buf;
    hdr->msgtid = RTMP_MSG_AMF_CMD;
    hdr->msgsid = h->msgsid;

    chain = rtmp_copy_buf_to_chain(session,buf);

    if (chain == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]prepare message failed!",
            session->sid);
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_ping_request(rtmp_session_t *session,uint32_t timestamp,
    rtmp_chunk_header_t *hdr)
{
    mem_buf_chain_t *chain;
    uint8_t         *last;

    chain = rtmp_create_proctol_chain(session,RTMP_MSG_USER,6,hdr);
    if (chain) {

        last = chain->chunk.last;

        ulong_make_byte2(last,RTMP_USER_PING_REQUEST);
        ulong_make_byte4(last+2,timestamp);
    }

    return chain;
}

mem_buf_chain_t* 
rtmp_create_ping_response(rtmp_session_t *session,uint32_t timestamp,
    rtmp_chunk_header_t *hdr)
{
    mem_buf_chain_t *chain;
    uint8_t         *last;

    chain = rtmp_create_proctol_chain(session,RTMP_MSG_USER,6,hdr);
    if (chain) {
        last = chain->chunk.last;

        ulong_make_byte2(last,RTMP_USER_PING_RESPONSE);
        ulong_make_byte4(last+2,timestamp);
    }

    return chain;
}