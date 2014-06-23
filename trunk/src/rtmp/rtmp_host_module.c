
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

typedef struct rtmp_hosts_ctx_s {
    list_t  allow_play;
    list_t  deny_play;

    list_t  allow_publish;
    list_t  deny_publish;

    array_t server_list;
}rtmp_hosts_ctx_t;

static void *rtmp_host_create_module(rtmp_cycle_t *cycle);
static int32_t rtmp_host_init_cycle(rtmp_cycle_t *cycle);
static int32_t rtmp_host_init_process(rtmp_cycle_t *cycle);
static int32_t rtmp_host_eixt_cycle(rtmp_cycle_t *cycle);

rtmp_module_t rtmp_host_moudle = {
    0,
    "rtmp_host_module",
    0,    
    rtmp_host_create_module,
    rtmp_host_init_cycle,
    rtmp_host_init_process,
    rtmp_host_eixt_cycle
};

static void *rtmp_host_create_module(rtmp_cycle_t *cycle)
{
    rtmp_hosts_ctx_t *ctx;
    mem_pool_t       *pool;

    pool = cycle->pool;
    ctx = mem_pcalloc(cycle->pool,sizeof(rtmp_hosts_ctx_t));
    if (ctx == NULL) {
        rtmp_log(RTMP_LOG_ERR,"create conf failed");
        return NULL;
    }

    if (array_init(&ctx->server_list,pool,10,sizeof(void *)) != RTMP_OK) {
        rtmp_log(RTMP_LOG_ERR,"create hosts failed");
        return NULL;
    }

    list_init(&ctx->allow_play);
    list_init(&ctx->allow_publish);
    list_init(&ctx->deny_play);
    list_init(&ctx->deny_publish);

    return (void *)ctx;
}

static int32_t rtmp_host_init_cycle(rtmp_cycle_t *cycle)
{
    rtmp_hosts_ctx_t *ctx;
    rtmp_conf_t       *conf,*it,*hostc,*appc,*apphead;
    char              *hostname,**word;
    mem_pool_t        *pool;
    rtmp_host_t       *host,**vhost;
    rtmp_app_conf_t  **app;
    rtmp_host_conf_t  *hconf;
    uint32_t           default_server;

    conf = rtmp_get_conf(cycle->conf,"rtmp",GET_CONF_CURRENT);
    if (conf == NULL) {
        return RTMP_FAILED;
    }

    ctx = rtmp_host_moudle.ctx;
    pool = cycle->pool;
    conf = rtmp_get_conf(conf,"server",GET_CONF_CHILD);
    if (conf == NULL) {
        return RTMP_FAILED;
    }

    it = conf;
    do {
        hostc = rtmp_get_conf(it,"server_name",GET_CONF_CHILD);
        default_server = 0;

        hostname = RTMP_HOSTNAME_DEF;
        if (hostc && (hostc->argv.nelts > 1)) {
            word = hostc->argv.elts;

            if ((hostc->argv.nelts > 2) && (strcmp(word[2],"default") == 0)) {
                default_server = 1;
            }

            hostname = word[1];
        }

        if (rtmp_host_conf_find(hostname,& ctx->server_list)) {
            rtmp_log(RTMP_LOG_WARNING,"server [%s] duplicate!",hostname);
            goto next_server;
        }
        
        host = mem_pcalloc(pool,sizeof(rtmp_host_t));
        if (host == NULL) {
            return RTMP_FAILED;
        }

        vhost = array_push(&ctx->server_list);
        if (!vhost) {
            rtmp_log(RTMP_LOG_ERR,"array:no more room!");
            return RTMP_FAILED;
        }
        strncpy(host->name,hostname,sizeof(host->name)-1);

        *vhost = host;

        hconf = mem_pcalloc(pool,sizeof(rtmp_host_conf_t));
        if (hconf == NULL) {
            rtmp_log(RTMP_LOG_ERR,"create host conf failed!");
            return RTMP_FAILED;
        }

        hconf->default_server = default_server;
        host->hconf = hconf;
        
        array_init(& host->apps,pool,10,sizeof(void*));
        
        if (rtmp_host_conf_init(cycle,hostc,host) != RTMP_OK) {
            rtmp_log(RTMP_LOG_ERR,"init host conf failed!");
            return RTMP_FAILED;
        }
        
        appc = rtmp_get_conf(hostc,"app",GET_CONF_NEXT);
        apphead = appc;

        do {
            if (appc == NULL) {
                break;
            }

            app = array_push(& host->apps);
            if (app == NULL) {
                rtmp_log(RTMP_LOG_ERR,"array no more room!");
                return RTMP_FAILED;
            }

            *app = mem_pcalloc(pool,sizeof(rtmp_app_conf_t));
            if (*app == NULL) {
                rtmp_log(RTMP_LOG_ERR,"alloc app failed!");
                return RTMP_FAILED;
            }

            appc = rtmp_get_conf(appc,"app",GET_CONF_NEXT);

            if (appc && appc->argv.nelts > 1) {
                rtmp_app_conf_init(cycle,appc,*app);
            }

        } while (appc != apphead);

    next_server:
        it = rtmp_get_conf(it,"server",GET_CONF_NEXT);
    } while (it && (it != conf));

    return RTMP_OK;
}

static int32_t rtmp_host_init_process(rtmp_cycle_t *cycle)
{
    return RTMP_OK;
}

static int32_t rtmp_host_eixt_cycle(rtmp_cycle_t *cycle)
{
    return RTMP_OK;
}

rtmp_host_t *rtmp_host_conf_find(char *name,array_t *a)
{
    uint32_t i;
    rtmp_host_t *conf;

    conf = NULL;
    if ((name == NULL) || (a == NULL) || (a->nelts == 0)) {
        return conf;
    }

    for (i = 0;i < a->nelts;i++) {
        conf = (rtmp_host_t *)((void **)a->elts)[i];
        if (conf && strcmp(conf->name,name) == 0) {
            break;
        }
        conf = NULL;
    }

    return conf;
}