
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __RTMP_CORE_CONF_H_INCLUDED__
#define __RTMP_CORE_CONF_H_INCLUDED__


#define RTMP_DEFAULT_PORT           1935
#define RTMP_DEFAULT_ACK            5000000
#define RTMP_DEFAULT_PING           5000 /*ms*/

#define RTMP_IPT_TYPE_PUBLISH       0x0001       
#define RTMP_IPT_TYPE_PLAY          0x0002

typedef struct rtmp_app_conf_s  rtmp_app_conf_t;
typedef struct rtmp_app_s       rtmp_app_t;

typedef struct rtmp_host_conf_s rtmp_host_conf_t;
typedef struct rtmp_host_s      rtmp_host_t;

typedef struct rtmp_ip_table_s rtmp_ip_table_t;

struct rtmp_ip_table_s {
    uint32_t    in_addr;
    uint32_t    mask;
    link_t      link;
};

struct rtmp_app_conf_s {
    uint32_t            push:1;
    uint32_t            pull:1;

    /*0 for play,1 for publish*/
    link_t              allow_list[2]; 
    link_t              deny_list[2];

    uint32_t            chunk_size;
    uint32_t            ack_size;
    uint32_t            ping_timeout;
    uint32_t            stream_buckets;
};

struct rtmp_host_conf_s {

    struct sockaddr     sock;
    socklen_t           socklen;

    uint32_t            default_server;

    uint32_t            ping;
    uint32_t            chunk_size;
    uint32_t            ack_size;

    /*0 for play,1 for publish*/
    link_t              allow_list[2];
    link_t              deny_list[2];

    link_t              allow_play_list;
    link_t              deny_play_list;
};

int32_t rtmp_host_conf_block(rtmp_cycle_t *c,rtmp_conf_t *conf);
int32_t rtmp_app_conf_block(rtmp_conf_t *conf,rtmp_app_t *app);

#endif
