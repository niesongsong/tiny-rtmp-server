
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __RTMP_CORE_CONF_H_INCLUDED__
#define __RTMP_CORE_CONF_H_INCLUDED__


#define RTMP_DEFAULT_PORT           1935
#define RTMP_DEFAULT_ACK            2500000
#define RTMP_DEFAULT_CHUNKSIZE      128
#define RTMP_DEFAULT_PING           5000 /*ms*/

typedef struct rtmp_app_conf_s rtmp_app_conf_t;
typedef struct rtmp_host_conf_s rtmp_host_conf_t;
typedef struct rtmp_host_s rtmp_host_t;

struct rtmp_app_conf_s {
    char        name[64];

    uint32_t    push:1;
    uint32_t    poll:1;
    uint32_t    exec:1;

};

struct rtmp_host_conf_s {
    char                name[64];

    struct sockaddr     sock;
    socklen_t           socklen;

    uint32_t            ping;
    uint32_t            ack;
    uint32_t            chunk;

    uint32_t            default_server;
};

int32_t rtmp_host_conf_init(rtmp_cycle_t *c,
    rtmp_conf_t *conf,rtmp_host_t *host);

int32_t rtmp_app_conf_init(rtmp_cycle_t *c,
    rtmp_conf_t *conf,rtmp_app_conf_t *app);

#endif
