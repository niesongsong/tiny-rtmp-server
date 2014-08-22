
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
    uint32_t                send_ping : 1;      /*ping flag*/

    uint32_t                in_bytes;
    uint32_t                in_last_ack;

    rtmp_handshake_t       *handshake;
    rtmp_session_connect_t *conn;

    rtmp_chunk_stream_t   **streams;
    uint32_t                stream_time;
    uint32_t                last_stream;
    uint32_t                max_streams;
    
    rtmp_connection_t      *c;
    mem_pool_t             *pool;       /* c->pool */
    mem_pool_t             *temp_pool;  /* cycle->temp_pool*/
    
    rtmp_live_link_t      **lives;
    uint32_t                max_lives;
    uint32_t                ping;
    uint32_t                ack_window;

    rtmp_host_t            *host_ctx;
    rtmp_app_t             *app_ctx;

    mem_pool_t             *chunk_pool;
    
    /*in*/
    uint32_t                in_chunk_size;
    mem_buf_chain_t        *in_chain;

    /*out*/
    mem_buf_chain_t       **out_message;    /*out message*/
    rtmp_chunk_header_t     last_sent;

    uint32_t                out_front;      /*queue front*/
    uint32_t                out_rear;       /*queue rear*/
    uint32_t                out_queue;      /*queue capacity*/

    mem_buf_chain_t        *out_chunk;      /*current chunk in current chain*/
    uint8_t                *out_last;       /*last out position*/
    uint32_t                out_chunk_size;
};

rtmp_session_t *rtmp_session_create(rtmp_connection_t *);
int32_t rtmp_session_destroy(rtmp_session_t *);

#endif
