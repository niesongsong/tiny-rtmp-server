
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

static void *rtmp_host_create_module(rtmp_cycle_t *cycle);
static int32_t rtmp_host_init_cycle(rtmp_cycle_t *cycle,rtmp_module_t *module);
static int32_t rtmp_host_init_process(rtmp_cycle_t *cycle,rtmp_module_t *module);
static int32_t rtmp_host_eixt_cycle(rtmp_cycle_t *cycle,rtmp_module_t *module);

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
    return (void *)(-1);
}

static int32_t rtmp_host_init_cycle(rtmp_cycle_t *cycle,
    rtmp_module_t *module)
{
    int32_t            rc;
    rtmp_conf_t       *conf,*it,*sconf;
    char             **word;

    conf = rtmp_get_conf(cycle->conf,"rtmp",GET_CONF_CURRENT);
    if (conf == NULL) {
        return RTMP_FAILED;
    }

    sconf = rtmp_get_conf(conf,"out_queue",GET_CONF_CHILD);
    if (sconf == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"no workers!");
    }

    if (sconf && sconf->argv.nelts > 1) {
        uint32_t          n;

        word = sconf->argv.elts;
        n = atoi(word[1]);
        if (n > 0) {
            cycle->out_queue = (uint32_t)n;
        }
    }

    conf = rtmp_get_conf(conf,"server",GET_CONF_CHILD);
    if (conf == NULL) {
        return RTMP_FAILED;
    }

    it = conf;
    do {
        rc = rtmp_host_conf_block(cycle,it);

        if (rc != RTMP_OK) {
            rtmp_log(RTMP_LOG_WARNING,"host conf failed [%d]!",rc);
            continue;
        }

        it = rtmp_get_conf(it,"server",GET_CONF_NEXT);
    } while (it && it != conf);

    return RTMP_OK;
}

static int32_t rtmp_host_init_process(rtmp_cycle_t *cycle,
    rtmp_module_t *module)
{
    return RTMP_OK;
}

static int32_t rtmp_host_eixt_cycle(rtmp_cycle_t *cycle,
    rtmp_module_t *module)
{
    return RTMP_OK;
}

rtmp_host_t *rtmp_host_conf_find(char *name,array_t *a)
{
    uint32_t i;
    rtmp_host_t *host;

    host = NULL;
    if ((name == NULL) || (a == NULL) || (a->nelts == 0)) {
        return host;
    }

    for (i = 0;i < a->nelts;i++) {
        host = (rtmp_host_t *)((void **)a->elts)[i];
        if (host && strcmp(host->name,name) == 0) {
            break;
        }
        host = NULL;
    }

    return host;
}