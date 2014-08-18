
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __RTMP_CORE_HANDLER__
#define __RTMP_CORE_HANDLER__

/* Protocol Message */
#define RTMP_MSG_CHUNK_SIZE         1
#define RTMP_MSG_ABORT              2
#define RTMP_MSG_ACK                3
#define RTMP_MSG_USER               4
#define RTMP_MSG_ACK_SIZE           5
#define RTMP_MSG_BANDWIDTH          6

#define RTMP_MSG_EDGE               7
#define RTMP_MSG_AUDIO              8
#define RTMP_MSG_VIDEO              9
#define RTMP_MSG_AMF3_META          15
#define RTMP_MSG_AMF3_SHARED        16
#define RTMP_MSG_AMF3_CMD           17
#define RTMP_MSG_AMF_META           18
#define RTMP_MSG_AMF_SHARED         19
#define RTMP_MSG_AMF_CMD            20
#define RTMP_MSG_AGGREGATE          22
#define RTMP_MSG_MAX                23

#define RTMP_USER_STREAM_BEGIN      0
#define RTMP_USER_STREAM_EOF        1
#define RTMP_USER_STREAM_DRY        2
#define RTMP_USER_SET_BUFLEN        3
#define RTMP_USER_RECORDED          4
#define RTMP_USER_PING_REQUEST      6
#define RTMP_USER_PING_RESPONSE     7
#define RTMP_USER_UNKNOWN           8
#define RTMP_USER_BUFFER_END        31


/*
    msg handler
 */
typedef int32_t (*rtmp_msg_handler_pt)(rtmp_session_t *s,
    rtmp_chunk_stream_t *st);

typedef struct rtmp_msg_handler_s {
    uint32_t            msg_id;
    rtmp_msg_handler_pt pt;
}rtmp_msg_handler_t;

int32_t rtmp_handler_null(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_chunksize(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_abort(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_ack(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_user(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_acksize(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_bandwidth(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_edge(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_avdata(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_avdata(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_amf3meta(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_amf3shared(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_amf3cmd(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_amfmeta(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_amfshared(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_amfcmd(rtmp_session_t *s,rtmp_chunk_stream_t *st);
int32_t rtmp_handler_aggregate(rtmp_session_t *s,rtmp_chunk_stream_t *st);

/*
    amf handler
*/
typedef int32_t (*rtmp_msg_amf_handler_pt)(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

typedef struct rtmp_msg_amf_handler_s {
    char                    *command;
    rtmp_msg_amf_handler_pt  pt;
}rtmp_msg_amf_handler_t;

int32_t rtmp_amf_cmd_connect(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_createstream(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_closestream(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_deletestream(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_releasestream(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_publish(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_play(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_play2(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_seek(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);

int32_t rtmp_amf_cmd_pause(rtmp_session_t *s,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num);


void rtmp_send_ping_response(rtmp_session_t *session,
    rtmp_chunk_stream_t *st,mem_buf_t *buf);

void rtmp_send_ping_request(rtmp_session_t *session);

#endif