
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

static int32_t rtmp_app_live_play_send(rtmp_session_t *session,
    rtmp_chunk_header_t *h,const char *livestream)
{
    int32_t             rc;

    rc = rtmp_create_append_chain(session,rtmp_create_set_chunk_size,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append set chunk size "
            "message failed!",session->sid);
        return RTMP_FAILED;
    }

    rc = rtmp_create_append_chain(session,rtmp_create_user_begin,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append user begin "
            "message failed!",session->sid);
        return RTMP_FAILED;
    }

    rc = rtmp_create_append_chain(session,rtmp_create_play_reset,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append play rest "
            "message failed!",session->sid);
        return RTMP_FAILED;
    }

    rc = rtmp_create_append_chain(session,rtmp_create_play_start,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append play start "
            "message failed!",session->sid);
        return RTMP_FAILED;
    }

    rc = rtmp_create_append_chain(session,rtmp_create_sample_access,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append sample access "
            "message failed!",session->sid);
        return RTMP_FAILED;
    }

    rtmp_chain_send(session->c->write);

    return RTMP_OK;
}

int32_t rtmp_app_live_play(rtmp_session_t *session,rtmp_chunk_stream_t *st,
    const char *livestream)
{
    rtmp_app_t              *app;
    rtmp_live_stream_t      *live;
    rtmp_live_link_t        *lvid;
    int32_t                  rc;
    rtmp_chunk_header_t     *h;

    h = & st->hdr;
    app = session->app_ctx;

    if ((h->msgsid == 0) || (h->msgsid >= session->max_lives) 
        || (session->lives[h->msgsid] == RTMP_NULL)) 
    {
        rc = rtmp_create_append_chain(session,rtmp_create_play_not_found,h);
        if (rc != RTMP_OK) {
            rtmp_log(RTMP_LOG_WARNING,"[%d]append play not found "
                "message failed!",session->sid);

            return RTMP_FAILED;
        }

        rtmp_log(RTMP_LOG_ERR,"[%d]\"%s\" stream not found!",
            session->sid,livestream);

        rtmp_chain_send(session->c->write);

        return RTMP_OK; 
    }

    lvid = mem_palloc(session->pool,sizeof(rtmp_live_link_t));
    if (lvid == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]\"%s\" alloc live link failed!",
            session->sid,livestream);
        return RTMP_FAILED;
    }
    lvid->start = 0;

    live = rtmp_app_live_get(app,livestream);
    if (live == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]\"%s\" get stream failed!",
            session->sid,livestream);
        return RTMP_FAILED;
    }

    rc = rtmp_app_live_play_send(session,h,livestream);
    if (rc == RTMP_OK) {

        session->lives[h->msgsid] = lvid;
        lvid->lsid = h->msgsid;
        lvid->session = session;
        lvid->lvst = live;

        list_insert_head(&live->players,&lvid->link); 
    }

    rtmp_log(RTMP_LOG_DEBUG,"[%d]play start[%d]",session->sid,rc);

    return RTMP_OK;
}

/*
    string "play"
    double 0.000000
    null
    string "cameraFeed"
*/
int32_t rtmp_amf_cmd_play(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    char  *livestream,*arg;

    if (num < 4 || amf[3] == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]amf num error[%d]!",s->sid,num);
        return RTMP_FAILED;
    }

    livestream = amf_get_string(amf[3]);
    arg = strchr(livestream,'?');
    if (arg != NULL) {
        *arg++ = 0;
        rtmp_log(RTMP_LOG_INFO,"publish args=\"%s\"",
            arg,(*arg ? arg:"null"));
    }

    return rtmp_app_live_play(s,st,livestream);
}