
/*
 * Copyright (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

#ifndef __HOST_H_INCLUDED__
#define __HOST_H_INCLUDED__

#define RTMP_HOSTNAME_DEF    "__defualt_host__"
#define RTMP_APPNAME_DEF     "__defualt_app__"

extern rtmp_module_t rtmp_hosts_moudle;

struct rtmp_host_s {
    char              name[128];
    rtmp_host_conf_t *hconf;
    array_t           apps;
    rtmp_cycle_t     *cycle;
};

rtmp_host_t *rtmp_host_conf_find(char *name,array_t *a);

#endif