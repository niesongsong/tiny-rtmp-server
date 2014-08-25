
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

rtmp_app_t* rtmp_create_app(mem_pool_t *pool,rtmp_host_t *host)
{
    mem_pool_t          *chunk_pool;
    rtmp_app_t          *app;
    rtmp_app_conf_t     *conf;
    rtmp_host_conf_t    *hconf;
    
    hconf = host->hconf;
    app = mem_pcalloc(pool,sizeof(rtmp_app_t) + sizeof(rtmp_app_conf_t));
    if (app == NULL) {
        return NULL;
    }

    chunk_pool = mem_create_pool(MEM_DEFAULT_POOL_SIZE);
    if (chunk_pool == NULL) {
        return NULL;
    }

    conf = (rtmp_app_conf_t*)(app + 1);
    app->conf = conf;
    app->chunk_pool = chunk_pool;

    conf->ack_size = hconf->ack_size;
    conf->ping_timeout = hconf->ping;
    conf->chunk_size = hconf->chunk_size;
    conf->stream_buckets = 1024;  

    list_init(&app->free_lives);

    app->host = host;

    return app;
}

rtmp_app_t *rtmp_app_conf_find(char *name,array_t *a)
{
    uint32_t i;
    rtmp_app_t *app;

    app = NULL;
    if ((name == NULL) || (a == NULL) || (a->nelts == 0)) {
        return app;
    }

    for (i = 0;i < a->nelts;i++) {
        app = (rtmp_app_t *)((void **)a->elts)[i];
        if (app && strcmp(app->name,name) == 0) {
            break;
        }
        app = NULL;
    }

    return app;
}

rtmp_live_stream_t* rtmp_app_live_find(rtmp_app_t *app,
    const char *livestream)
{
    rtmp_live_stream_t  *live;
    list_t              *h,*next;
    uint32_t             k;

    k = rtmp_hash_string(livestream);
    h = app->lives + (k % app->conf->stream_buckets);

    next = h->next;
    while (next != h) {

        live = struct_entry(next,rtmp_live_stream_t,link);
        if (strcmp(livestream,live->name) == 0) {
            break;
        }
        next = next->next;
    }

    if (next == h) {
        return NULL;
    }

    return live;
}

rtmp_live_stream_t* rtmp_app_live_alloc(rtmp_app_t *app,
    const char *livestream)
{

    mem_pool_t              *pool;
    rtmp_live_stream_t      *live;
    uint32_t                 k;
    list_t                  *h;

    if (list_empty(&app->free_lives) == 0) {
        
        live = struct_entry(app->free_lives.next,
            rtmp_live_stream_t,link);

        list_remove(&live->link);
    } else {
        pool = app->host->cycle->pool;
        live = mem_pcalloc(pool,sizeof(rtmp_live_stream_t));
    }

    if (live) {
        live->epoch = rtmp_current_sec;
        live->timestamp = 0;
        strncpy(live->name,livestream,63);

        live->publisher = NULL;
        live->players = NULL;

        k = rtmp_hash_string(livestream);
        h = app->lives + (k % app->conf->stream_buckets);

        list_insert_head(h,&live->link);
    }

    return live;
}

void rtmp_app_live_free(rtmp_app_t *app,rtmp_live_stream_t *live)
{
    list_remove(&live->link);

#ifdef HAVE_DEBUG
    memset(live,0,sizeof(rtmp_live_stream_t));
#endif

    list_insert_head(&app->free_lives,&live->link);
}

void rtmp_app_live_release(rtmp_live_link_t *link)
{
    rtmp_live_stream_t *live;

    live = link->lvst;

    if (live->publisher == link) {
        live->publisher = NULL;
    } else {
        list_remove(&link->link);
        if (link == live->players) {
            live->players = NULL;
        }
    }

    if ((live->publisher == NULL) && (live->players == NULL)) {

        rtmp_log(RTMP_LOG_DEBUG,"[%d]free live[%s]",
            link->session->sid,live->name);

        rtmp_app_live_free(link->session->app_ctx,live);
    }
    return ;
}

rtmp_live_stream_t* rtmp_app_live_get(rtmp_app_t *app,
    const char *livestream)
{
    rtmp_live_stream_t *live;

    live = rtmp_app_live_find(app,livestream);
    if (live == NULL) {
        live = rtmp_app_live_alloc(app,livestream);
    }

    return live;
}