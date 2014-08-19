
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

#ifndef __APP_H_INCLUDED__
#define __APP_H_INCLUDED__

#define RTMP_STREAM_LIVE        0
#define RTMP_STREAM_RECORD      1
#define RTMP_STREAM_APPEND      2

typedef struct rtmp_live_stream_s     rtmp_live_stream_t;
typedef struct rtmp_live_link_s       rtmp_live_link_t;

struct rtmp_live_link_s {
    rtmp_session_t         *session;
    rtmp_live_stream_t     *lvst;
    uint32_t                start:1;
    uint32_t                lsid;

    link_t                  link;
};

struct rtmp_live_stream_s {
    char                name[64];
    uint32_t            epoch;
    rtmp_live_link_t   *publisher;
    list_t              players;
    link_t              link;
};

struct rtmp_app_s {
    char                 name[128];
    rtmp_app_conf_t     *conf;
    rtmp_host_t         *host;
    mem_pool_t          *chunk_pool;

    list_t              *lives;      /*lives ptr,len: stream_buckets*/
    list_t               free_lives; /*live free list*/
};

rtmp_app_t* rtmp_create_app(mem_pool_t *pool,rtmp_host_t *hconf);
rtmp_app_t* rtmp_app_conf_find(char *name,array_t *a);

rtmp_live_stream_t* rtmp_app_live_find(rtmp_app_t *app,
    const char *livestream);

rtmp_live_stream_t* rtmp_app_live_alloc(rtmp_app_t *app,
    const char *livestream);

/*
maybe

static rtmp_live_stream_t *rtmp_app_live_get(rtmp_app_t *app,
        const char *livestream) 
{
    rtmp_live_stream_t *live;

    live = rtmp_app_live_find(app,livestream);
    if (live == NULL) {
        live = rtmp_app_live_alloc(app,livestream);
    }
    return live;
}

*/
rtmp_live_stream_t* rtmp_app_live_get(rtmp_app_t *app,
    const char *livestream);

int32_t rtmp_app_live_publish(rtmp_session_t *session,
    rtmp_chunk_stream_t *h,char *livestream);

int32_t rtmp_app_live_play(rtmp_session_t *session,
    rtmp_chunk_stream_t *h,const char *livestream);


void rtmp_app_live_free(rtmp_app_t *app,rtmp_live_stream_t *live);

void rtmp_app_live_release(rtmp_live_link_t *link);

#endif