
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

int32_t rtmp_amf_cmd_createstream(rtmp_session_t *session,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    amf_data_t             *amf_response[4];
    mem_buf_t              *buf; 
    mem_buf_chain_t        *chain;
    uint32_t                lsid;
    
    if (num != 3) {
        rtmp_log(RTMP_LOG_ERR,"amf number[%d] error!",num);
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

    if (st->hdr.msgtid == RTMP_MSG_AMF3_CMD) {
        st->hdr.msgtid = RTMP_MSG_AMF_CMD;
    }

    chain = rtmp_prepare_memssage_buf(session,&st->hdr,buf);
    if (chain == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"prepare connect app message failed!");
        return RTMP_FAILED;
    }

    if (rtmp_append_message_chain(session,chain) == -1) {
        rtmp_log(RTMP_LOG_WARNING,"append connect app message failed!");
        return RTMP_FAILED;
    }

    rtmp_chain_send(session->c->write);
    return RTMP_OK;
}

int32_t rtmp_amf_cmd_releasestream(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    return RTMP_OK;
}
