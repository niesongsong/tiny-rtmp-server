
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

#define RTMP_PACKET_TYPE_AUDIO 0x08
#define RTMP_PACKET_TYPE_VIDEO 0x09
#define RTMP_PACKET_TYPE_AMF0  0x12
#define RTMP_PACKET_TYPE_AMF3  0x0f

#define RTMP_OK             (0)
#define RTMP_NULL           (NULL)

#define RTMP_ERROR          (-1)
#define RTMP_FAILED         (-1)
#define RTMP_TIMEOUT        (-2)
#define RTMP_BLOCK          (-3)
#define RTMP_RESET          (-4)
#define RTMP_HANDSHAKE      (-5)
#define RTMP_PROTOCOL       (-6)
#define RTMP_INVALID_URL    (-7)

typedef struct rtmp_packet_s rtmp_packet_t;
typedef void * rtmp_t;

struct rtmp_packet_s {
    uint32_t    timestamp;
    uint8_t     type;
    uint8_t    *payload;
    uint16_t    size;
};

/*memory alloc and free instance*/
typedef void* (*rtmp_malloc_rt)(size_t size,void *u);
typedef void  (*rtmp_free_rt)(void *p,void *u);

/*constructor and destructor*/
XRTMP_API rtmp_t rtmp_create(const char *url,
    rtmp_malloc_rt a,rtmp_free_rt f,void *u);

XRTMP_API void   rtmp_destroy(rtmp_t rtmp);

/*handshake with server*/
XRTMP_API int rtmp_handshake(rtmp_t rtmp);
XRTMP_API int rtmp_handshake_v2(rtmp_t rtmp);

/*connect application*/
XRTMP_API int rtmp_connect_app(rtmp_t rtmp);

/*play/publish stream*/
XRTMP_API int rtmp_play(rtmp_t rtmp);
XRTMP_API int rtmp_publish(rtmp_t rtmp);

/*send/recv packet from server*/
XRTMP_API int rtmp_send(rtmp_t rtmp,const rtmp_packet_t *pkt);
XRTMP_API int rtmp_recv(rtmp_t rtmp,rtmp_packet_t *pkt);

#endif