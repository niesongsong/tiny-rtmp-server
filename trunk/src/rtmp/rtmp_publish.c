
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

int32_t rtmp_app_live_publish(rtmp_session_t *session,
    rtmp_chunk_header_t *chunk,char *livestream)
{
    rtmp_app_t              *app;
    rtmp_live_stream_t      *live;
    rtmp_live_link_t        *lvid;
    int32_t                  rc;
    rtmp_chunk_header_t     *h;

    h = chunk;
    app = session->app_ctx;

    if ((h->msgsid == 0) || (h->msgsid >= session->max_lives) 
     || (session->lives[h->msgsid] == RTMP_NULL)) 
    {
        rc = rtmp_create_append_chain(session,rtmp_create_publish_not_found,h);
        if (rc != RTMP_OK) {
            rtmp_log(RTMP_LOG_WARNING,"[%d]append publish not found "
                "message failed!",session->sid);

            return RTMP_FAILED;
        }

        rtmp_log(RTMP_LOG_ERR,"[%d]\"%s\" stream not found!",
            session->sid,livestream);

        rtmp_chain_send(session->c->write);

        return RTMP_OK; 
    }
    lvid = session->lives[h->msgsid];

    live = rtmp_app_live_find(app,livestream);
    if ((lvid != RTMP_READY) || (live && live->publisher)){

        rc = rtmp_create_append_chain(session,rtmp_create_publish_badname,h);
        if (rc != RTMP_OK) {
            rtmp_log(RTMP_LOG_WARNING,"[%d]append publish badname "
                "message failed!",session->sid);
            return RTMP_FAILED;
        }

        rtmp_log(RTMP_LOG_ERR,"[%d]\"%s\" already publish!",
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

    if (live == NULL) {
        live = rtmp_app_live_alloc(app,livestream);
        if (live == NULL) {
            rtmp_log(RTMP_LOG_ERR,"[%d]alloc live failed!",session->sid);
            return RTMP_FAILED;
        }
    }

    rc = rtmp_create_append_chain(session,rtmp_create_set_chunk_size,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append set chunk size "
            "message failed!",session->sid);

        return RTMP_FAILED;
    }

    rc = rtmp_create_append_chain(session,rtmp_create_publish_start,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append publish start "
            "message failed!",session->sid);

        return RTMP_FAILED;
    }

    lvid->session = session;
    lvid->lvst = live;
    lvid->msgid = h->msgsid;

    session->lives[h->msgsid] = lvid;
    live->publisher = lvid;

    rtmp_log(RTMP_LOG_DEBUG,"[%d]\"%s\" publish starting!",
        session->sid,livestream);

    rtmp_chain_send(session->c->write);

    return RTMP_OK;
}

/*
    string "publish"
    double 0.000000
    null
    string "cameraFeed"
    string "live"
*/
int32_t rtmp_amf_cmd_publish(rtmp_session_t *s,
    rtmp_chunk_header_t *chunk,amf_data_t *amf[],uint32_t num)
{
    char  *livestream,*streamtype,*arg;

    if (num < 5) {
        rtmp_log(RTMP_LOG_ERR,"[%d]amf num error[%d]!",s->sid,num);
        return RTMP_FAILED;
    }

    if (amf[3] == NULL || amf[4] == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]amf null error[%p][%p]!",
            s->sid,amf[3],amf[4]);
        return RTMP_FAILED;
    }

    streamtype = amf_get_string(amf[4]);
    if ((streamtype == NULL) || (strcmp(streamtype,"live"))) {
        rtmp_log(RTMP_LOG_ERR,"[%d]stream type[%s]!",
            s->sid,streamtype ? streamtype:"null");
        return RTMP_FAILED;
    }

    livestream = amf_get_string(amf[3]);
    arg = strchr(livestream,'?');
    if (arg != NULL) {
        *arg++ = 0;
        rtmp_log(RTMP_LOG_INFO,"publish args=\"%s\"",
            arg,(*arg ? arg:"null"));
    }

    return rtmp_app_live_publish(s,chunk,livestream);
}