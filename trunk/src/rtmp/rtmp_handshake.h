
/*
 * Copyright (C) nie950@gmail.com
 */


#ifndef __HANDSHAKE_H_INCLUDED__
#define __HANDSHAKE_H_INCLUDED__


#define HANDSHAKE_BUF_LEN   1537
#define HANDSHAKE_KEY_LEN   32

typedef struct rtmp_handshake_s{

    mem_buf_t           rbuf;
    mem_buf_t           wbuf;

    u_char             *digest;

    uint32_t            stage:4;
    
    uint32_t            epoch;
    uint32_t            peer_epoch;
    char                peer_version[4];
} rtmp_handshake_t;

void rtmp_core_handshake_init();
void rtmp_handshake_recv(rtmp_event_t *ev);
void rtmp_handshake_send(rtmp_event_t *ev);
int32_t rtmp_handshake_alloc(mem_pool_t *,rtmp_handshake_t *);

#endif