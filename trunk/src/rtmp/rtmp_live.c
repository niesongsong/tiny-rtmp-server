
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


static int32_t rtmp_live_send_avdata(rtmp_live_link_t *client,
rtmp_chunk_stream_t *st,mem_buf_chain_t *chain);

static int32_t rtmp_live_send_avdata(rtmp_live_link_t *client,
    rtmp_chunk_stream_t *st,mem_buf_chain_t *chain)
{
    rtmp_session_t     *session;

    session = client->session;
    if (rtmp_append_message_chain(session,chain) != RTMP_OK) {
        rtmp_core_free_chains(session,session->chunk_pool,chain);
        rtmp_log(RTMP_LOG_DEBUG,"[%d] drop message",session->sid);
    }
    
    rtmp_core_lock_chain(chain);

    rtmp_chain_send(client->session->c->write);
    return RTMP_OK;
}

int32_t rtmp_handler_avdata(rtmp_session_t *session,rtmp_chunk_stream_t *st)
{
    rtmp_live_stream_t     *live;
    rtmp_live_link_t       *link,*client;
    link_t                 *next;
    int32_t                 rc;
    mem_buf_chain_t        *chain;

    if ((st->hdr.msgsid == 0) || (st->hdr.msgsid >= session->max_lives)) {
        rtmp_log(RTMP_LOG_ERR,"[%d]invalid message stream id[%d]!",
            session->sid,st->hdr.msgsid);
        return RTMP_FAILED;
    }

    link = session->lives[st->hdr.msgsid];
    if (link == RTMP_NULL || link == RTMP_READY) {
        rtmp_log(RTMP_LOG_ERR,"[%d]invalid message stream status[%p]!",
            session->sid,link);
        return RTMP_FAILED;
    }

    if (link->lvst == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]invalid live (null)!",session->sid);
        return RTMP_FAILED;
    }

    if (link != link->lvst->publisher) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]not publisher!",session->sid);
        return RTMP_OK;
    }

    chain = rtmp_prepare_memssage_chain(session,&st->hdr,st->chain);
    if (chain == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]crtmp_message_copy_to_chain() failed!",
            session->sid);
        return RTMP_FAILED;
    }

    live = link->lvst;
    next = live->players.next;

    /*broad cast*/
    while (next != &live->players) {
        client = struct_entry(next,rtmp_live_link_t,link);
        next = client->link.next;

        if (client->session == NULL) {
            list_remove(&client->link);
            rtmp_log(RTMP_LOG_WARNING,"[%d]invalid session!",
                session->sid);
            continue;
        }

        rc = rtmp_live_send_avdata(client,st,chain);

        if (rc != RTMP_OK) {
            rtmp_log(RTMP_LOG_WARNING,"[%d]send avdata[%d]",
                client->session->sid,rc);
            continue;
        }

        rtmp_log(RTMP_LOG_DEBUG,"[%d]send avdata",
            client->session->sid);
    }

    rtmp_core_free_chains(session,session->chunk_pool,chain);

    return RTMP_OK;
}


