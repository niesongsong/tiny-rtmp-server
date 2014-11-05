
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

int32_t rtmp_amf_cmd_createstream(rtmp_session_t *session,
    rtmp_chunk_header_t *hdr,amf_data_t *amf[],uint32_t num)
{
    amf_data_t             *amf_response[4];
    mem_buf_t              *buf; 
    mem_buf_chain_t        *chain;
    uint32_t                lsid;
    rtmp_chunk_header_t     h;

    h = *hdr;
    
    if (num != 3) {
        rtmp_log(RTMP_LOG_ERR,"[%d]amf number[%d] error!",session->sid,num);
        return RTMP_FAILED;
    }

    if (session->app_ctx == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]connect failed!",session->sid);
        return RTMP_FAILED;
    }

    for (lsid = 1;lsid <  session->max_lives;lsid++) {
        if (session->lives[lsid] == RTMP_NULL) {
            break;
        }
    }

    if (lsid == session->max_lives) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]get empty stream id failed!",
            session->sid);
        return RTMP_FAILED;
    }

    session->lives[lsid] = RTMP_READY;

    amf_response[0] = amf_new_string("_result",0);
    amf_response[1] = amf[1];
    amf_response[2] = amf_new_null();
    amf_response[3] = amf_new_number(lsid);

    buf = rtmp_prepare_amf_buffer(session->temp_pool,amf_response,4);
    if (buf == NULL) {
        return RTMP_FAILED;
    }

    if (h.msgtid == RTMP_MSG_AMF3_CMD) {
        h.msgtid = RTMP_MSG_AMF_CMD;
    }

    chain = rtmp_copy_buf_to_chain(session,buf);

    if (chain == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]prepare connect app message failed!",
            session->sid);
        return RTMP_FAILED;
    }

    h.msglen = buf->last - buf->buf;
    h.fmt = 0;

    if (rtmp_append_message_chain(session,chain,&h) == -1) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append connect app message failed!",
            session->sid);
        return RTMP_FAILED;
    }

    rtmp_chain_send(session->c->write);
    return RTMP_OK;
}

int32_t rtmp_amf_cmd_releasestream(rtmp_session_t *session,
    rtmp_chunk_header_t *chunk,amf_data_t *amf[],uint32_t num)
{
    return RTMP_OK;
}
