
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __RTMP_SESSION_H_INCLUDED__
#define __RTMP_SESSION_H_INCLUDED__

#define RTMP_DEFAULT_MAX_STREAMS         32
#define RTMP_DEFAULT_MAX_LIVES           8
#define RTMP_CONN_APPNAME_SIZE_MAX       256
#define RTMP_CONN_URL_SIZE_MAX           256
#define RTMP_CONN_ARGS_SIZE_MAX          256
#define RTMP_CONN_VER_SIZE_MAX           32

typedef struct rtmp_session_connect_s rtmp_session_connect_t;
struct rtmp_session_connect_s {
    char    *app;
    char    *args;
    char    *flashver;
    char    *swf_url;
    char    *tc_url;
    char    *page_url;
    char    *vhost;
    double   trans;
    double   acodecs;
    double   vcodecs;
    double   object_encoding; 
};

struct rtmp_session_s {
    uint32_t                sid;                /*session id*/
    
    /*handshake*/
    rtmp_handshake_t       *handshake;
    rtmp_session_connect_t *conn;

    /*chunk info*/
    rtmp_chunk_stream_t   **chunk_streams;
    uint32_t                chunk_time;
    uint32_t                max_streams;
        
    /*lives info*/
    rtmp_live_link_t      **lives;          /*0 is reserved*/
    uint32_t                max_lives;

    /*chain in*/
    uint32_t                in_chunk_size;
    mem_buf_chain_t        *in_chain;

    /*chain out*/
    mem_buf_chain_t       **out_chain;      /*out chain*/
    uint32_t                out_front;      /*queue front*/
    uint32_t                out_rear;       /*queue rear*/
    uint32_t                out_queue;      /*queue capacity*/
    mem_buf_chain_t        *out_chunk;      /*current chunk in current chain*/
    uint8_t                *out_last;       /*last out position*/
    uint32_t                out_chunk_size;

    /*connection info*/
    rtmp_connection_t      *c;
    mem_pool_t             *pool;
    mem_pool_t             *temp_pool;
    mem_pool_t             *chunk_pool;
    rtmp_host_t            *host_ctx;
    rtmp_app_t             *app_ctx;
    uint32_t                ping_timeout;
    uint32_t                ping_sent : 1;
    uint32_t                ack_window;
    uint32_t                in_bytes;
    uint32_t                in_last_ack;
};

rtmp_session_t *rtmp_session_create(rtmp_connection_t *);
int32_t rtmp_session_destroy(rtmp_session_t *);

#endif
