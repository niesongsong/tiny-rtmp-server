
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __MESSAGE_H_INCLUDED__
#define __MESSAGE_H_INCLUDED__

typedef mem_buf_chain_t* (*rtmp_create_proctol_message_ptr)
    (rtmp_session_t *session,rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_ack_size(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_ack(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_peer_bandwidth_size(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_set_chunk_size(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_user_begin(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_play_reset(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_play_start(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_publish_start(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_publish_badname(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_sample_access(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_play_not_found(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_publish_not_found(rtmp_session_t *session,
    rtmp_chunk_header_t *h);

mem_buf_chain_t* rtmp_create_ping_request(rtmp_session_t *session,
    uint32_t timestamp);

mem_buf_chain_t* rtmp_create_ping_response(rtmp_session_t *session,
    uint32_t timestamp);

mem_buf_chain_t* rtmp_prepare_memssage_buf(rtmp_session_t *session,
    rtmp_chunk_header_t *sthead,mem_buf_t *message);

mem_buf_chain_t* rtmp_prepare_memssage_chain(rtmp_session_t *session,
    rtmp_chunk_header_t *sthead,mem_buf_chain_t *chain);

int32_t rtmp_create_append_chain(rtmp_session_t *session,
    rtmp_create_proctol_message_ptr ptr,rtmp_chunk_header_t *h);

int32_t rtmp_append_message_chain(rtmp_session_t *session,
    mem_buf_chain_t *chain);

#endif