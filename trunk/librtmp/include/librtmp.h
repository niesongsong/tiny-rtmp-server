
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __LIB_RTMP_H_INCLUDED__
#define __LIB_RTMP_H_INCLUDED__

#if defined(_WIN32) && !defined(__GNUC__)  && defined(XRTMPDLL)
#if defined(LIBRTMP_EXPORTS)
#define XRTMP_API __declspec(dllexport)
#else
#define XRTMP_API __declspec(dllimport)
#endif
#else
#define XRTMP_API
#endif

#define rtmp_proto_n            "rtmp://"
#define rtmp_proto_len          (sizeof(rtmp_proto_n)-1)

#define RTMP_MESSAGE_TYPE_AUDIO  0x08
#define RTMP_MESSAGE_TYPE_VIDEO  0x09
#define RTMP_MESSAGE_TYPE_AMF0   0x12
#define RTMP_MESSAGE_TYPE_AMF3   0x0f

#define RTMP_NULL               (NULL)

#define RTMP_ERR_OK             ( 0)
#define RTMP_ERR_ERROR          (-1)
#define RTMP_ERR_FAILED         (-1)
#define RTMP_ERR_TIMEOUT        (-2)
#define RTMP_ERR_BLOCK          (-3)
#define RTMP_ERR_RESET          (-4)
#define RTMP_ERR_HANDSHAKE      (-5)
#define RTMP_ERR_PROTOCOL       (-6)
#define RTMP_ERR_INVALID        (-7)

#define RTMP_LOG_ERR            ( 0)
#define RTMP_LOG_WARNING        ( 1)
#define RTMP_LOG_INFO           ( 2)
#define RTMP_LOG_NORMAL         ( 3)
#define RTMP_LOG_DEBUG          ( 4)
#define RTMP_LOG_MAX            (RTMP_LOG_DEBUG+1)

#define RTMP_CONN_BLOCK         ( 1)
#define RTMP_CONN_NONEBLOCK     ( 0)

typedef struct rtmp_message_s rtmp_message_t;
typedef void * rtmp_net_conn_t;
typedef void * rtmp_net_stream_t;

typedef unsigned int   uint32_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;

struct rtmp_message_s {
    uint32_t    timestamp;
    uint16_t    type;
    uint16_t    body_size;
    uint8_t    *body;
};

/*memory alloc and free instance*/
typedef void* (*rtmp_malloc_rt)(size_t size,void *u);
typedef void  (*rtmp_free_rt)(void *p,void *u);

/*NetConnection*/
XRTMP_API rtmp_net_conn_t rtmp_conn_create(
    rtmp_malloc_rt a,rtmp_free_rt f,void *u);

XRTMP_API int rtmp_conn_connect(rtmp_net_conn_t conn,const char *app);
XRTMP_API int rtmp_conn_close(rtmp_net_conn_t conn);

XRTMP_API int rtmp_conn_destroy(rtmp_net_conn_t conn);

/*NetStream*/
XRTMP_API rtmp_net_stream_t rtmp_stream_create(rtmp_net_conn_t conn,
    const char *name);
XRTMP_API int rtmp_stream_play(rtmp_net_stream_t stream);
XRTMP_API int rtmp_stream_publish(rtmp_net_stream_t stream);

XRTMP_API int rtmp_stream_recv(rtmp_net_stream_t stream,rtmp_message_t *msg);
XRTMP_API int rtmp_stream_send(rtmp_net_stream_t stream,rtmp_message_t *msg);

XRTMP_API int rtmp_stream_close(rtmp_net_stream_t stream);
XRTMP_API int rtmp_stream_destroy(rtmp_net_stream_t stream);

#endif