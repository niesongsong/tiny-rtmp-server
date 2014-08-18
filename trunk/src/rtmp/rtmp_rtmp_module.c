
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


static void *rtmp_core_create_module(rtmp_cycle_t *cycle);
static int32_t rtmp_core_init_cycle(rtmp_cycle_t *cycle);
static int32_t rtmp_core_init_process(rtmp_cycle_t *cycle);
static int32_t rtmp_core_eixt_cycle(rtmp_cycle_t *cycle);

extern int rtmp_daemon_mode;

rtmp_module_t rtmp_core_moudle = {
    0,
    "rtmp_core_module",
    0,
    rtmp_core_create_module,
    rtmp_core_init_cycle,
    rtmp_core_init_process,
    rtmp_core_eixt_cycle
};

static void *rtmp_core_create_module(rtmp_cycle_t *cycle)
{
    rtmp_core_conf_t *conf;

    conf = mem_pcalloc(cycle->pool,sizeof(rtmp_core_conf_t));
    if (conf == NULL) {
        rtmp_log(RTMP_LOG_ERR,"create conf failed");
    }

    return conf;
}

static int32_t rtmp_core_init_cycle(rtmp_cycle_t *cycle)
{
    rtmp_conf_t      *conf;
    char            **word;
    rtmp_core_conf_t *cconf;

    conf = rtmp_get_conf(cycle->conf,"daemon",GET_CONF_NEXT);
    cconf = rtmp_core_moudle.conf;

    cconf->daemon = CONF_OFF;

#ifndef HAVE_OS_WIN32 /*win32 doesn't support daemon*/

    if (conf && (conf->argv.nelts > 1)) {
        word = conf->argv.elts;
        if ((word[1][0] == 'o') && (word[1][1] == 'n')) {
            cconf->daemon = CONF_ON;
        } 
    }

#endif

    rtmp_daemon_mode = cconf->daemon;

    cconf->workers = 1;
    if (cconf->daemon == CONF_ON) {
        conf = rtmp_get_conf(cycle->conf,"workers",GET_CONF_NEXT);

        if (conf && (conf->argv.nelts > 1)) {
            word = conf->argv.elts;
            cconf->workers = atoi(word[1]);
        }

        if (cconf->workers == 0) {
            cconf->workers = 1;
        }
    }

    return RTMP_OK;
}

static int32_t rtmp_core_init_process(rtmp_cycle_t *cycle)
{
    return RTMP_OK;
}

static int32_t rtmp_core_eixt_cycle(rtmp_cycle_t *cycle)
{
    return RTMP_OK;
}