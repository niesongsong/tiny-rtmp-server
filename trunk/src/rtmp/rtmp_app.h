
/*
 * Copyright (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

#ifndef __APP_H_INCLUDED__
#define __APP_H_INCLUDED__

#define RTMP_STREAM_LIVE        0
#define RTMP_STREAM_RECORD      1
#define RTMP_STREAM_APPEND      2

typedef struct rtmp_app_s rtmp_app_t;
struct rtmp_app_s {
    rtmp_app_conf_t     *conf;
    mem_pool_t          *pool;
};

#endif