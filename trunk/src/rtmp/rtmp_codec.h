
/*
 * CopyLeft (C) nie950@gmail.com
 */

#define RTMP_STREAM_LIVE        0
#define RTMP_STREAM_RECORD      1
#define RTMP_STREAM_APPEND      2

typedef struct rtmp_live_codec_s      rtmp_live_codec_t;

struct rtmp_live_codec_s {
    /*video codec*/
    uint32_t    video_codec_id;
    uint32_t    width;
    uint32_t    height;
    uint32_t    duration;
    uint32_t    frame_rate;
    uint32_t    video_data_rate;
    uint32_t    avc_version;
    uint32_t    avc_profile;
    uint32_t    avc_compat;
    uint32_t    avc_level;
    uint32_t    avc_nal_bytes;
    uint32_t    avc_ref_frames;

    


    /*audio codec*/
    uint32_t    audio_codec_id;
    uint32_t    audio_data_rate;
    uint32_t    aac_profile;
    uint32_t    aac_chan_conf;
    uint32_t    aac_sbr;
    uint32_t    aac_ps;
    uint32_t    sample_rate;
    uint32_t    sample_size;
    uint32_t    audio_channels;

    u_char      profile[32];
    u_char      level[32];

    /*meta data*/
    mem_buf_chain_t *meta;
};