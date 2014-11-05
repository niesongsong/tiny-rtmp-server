
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __MESSAGE_H_INCLUDED__
#define __MESSAGE_H_INCLUDED__

typedef struct rtmp_message_s rtmp_message_t;
struct rtmp_message_s {
    rtmp_chunk_header_t  hdr;
    mem_buf_t            head;
    mem_buf_chain_t     *chain;
    uint8_t              head_buf[RTMP_MAX_CHUNK_HEADER];
};

typedef mem_buf_chain_t* (*rtmp_create_proctol_message_ptr)
    (rtmp_session_t *session,
     rtmp_chunk_header_t *ih,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_ack_size(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_ack(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_peer_bandwidth_size(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_set_chunk_size(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_user_begin(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_play_reset(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_play_start(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_publish_start(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_publish_badname(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_sample_access(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_play_not_found(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_publish_not_found(rtmp_session_t *session,
    rtmp_chunk_header_t *h,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_ping_request(rtmp_session_t *session,
    uint32_t timestamp,rtmp_chunk_header_t *oh);

mem_buf_chain_t* rtmp_create_ping_response(rtmp_session_t *session,
    uint32_t timestamp,rtmp_chunk_header_t *oh);


int32_t rtmp_create_append_chain(rtmp_session_t *session,
    rtmp_create_proctol_message_ptr ptr,rtmp_chunk_header_t *h);

int32_t rtmp_append_message_chain(rtmp_session_t *session,
    mem_buf_chain_t *chain,rtmp_chunk_header_t *h);

mem_buf_t* rtmp_copy_chain_to_buf(mem_buf_chain_t *chain_in,
    mem_pool_t *temp_pool);

mem_buf_chain_t *rtmp_copy_buf_to_chain(rtmp_session_t *session,
    mem_buf_t *buf);

mem_buf_chain_t *rtmp_copy_chain_to_chain(rtmp_session_t *session,
    mem_buf_chain_t *in_chain);

#endif