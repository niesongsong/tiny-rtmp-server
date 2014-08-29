
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

static uint32_t aac_sample_rates[] = { 
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025,  8000,
    7350,     0,     0,     0 
};

/* The Audio Specific Config is the global header for MPEG-4 Audio
   5 bits: object type
   if (object type == 31)
     6 bits + 32: object type
   4 bits: frequency index
   if (frequency index == 15)
     24 bits: frequency
   4 bits: channel configuration

   if (object_type == 5)
       4 bits: frequency index
       if (frequency index == 15)
         24 bits: frequency
       5 bits: object type
       if (object type == 31)
         6 bits + 32: object type

   var bits: AOT Specific Config
*/
uint32_t rtmp_parse_aac_header(rtmp_session_t *session,mem_buf_chain_t *chain,
    rtmp_live_codec_t *codec)
{
    mem_buf_t           *buf;
    mem_bits_t           bitstream;
    uint32_t             idx;

    buf = rtmp_copy_chains_to_temp_buf(chain,session->temp_pool);
    if (buf == NULL) {
        return RTMP_FAILED;
    }
    mem_bits_init(&bitstream,buf);

    /*skip flv head*/
    mem_bits_read(&bitstream,16);

    codec->aac_profile = mem_bits_read(&bitstream,5);
    if (codec->aac_profile == 31) {
        codec->aac_profile = mem_bits_read(&bitstream,6) + 32;
    }

    idx = mem_bits_read(&bitstream,4);
    if (idx == 15) {
        codec->frame_rate = mem_bits_read(&bitstream,24);
    } else {
        codec->frame_rate = aac_sample_rates[idx];
    }

    codec->aac_chan_conf = mem_bits_read(&bitstream, 4);
    
    if (codec->aac_profile == 5 || codec->aac_profile == 29) {

        if (codec->aac_profile == 29) {
            codec->aac_ps = 1;
        }

        codec->aac_sbr = 1;
        idx = mem_bits_read(&bitstream, 4);
        if (idx == 15) {
            codec->sample_rate = mem_bits_read(&bitstream, 24);
        } else {
            codec->sample_rate = aac_sample_rates[idx];
        }

        codec->aac_profile = mem_bits_read(&bitstream, 5);
        if (codec->aac_profile == 31) {
            codec->aac_profile = mem_bits_read(&bitstream, 6) + 32;
        }
    }

    rtmp_log(RTMP_LOG_DEBUG,"codec: "
        "aac header profile=%u,sample_rate=%u, chan_conf=%u",
        codec->aac_profile, codec->sample_rate, codec->aac_chan_conf);

    return RTMP_OK;
}

uint32_t rtmp_parse_avc_header(rtmp_session_t *session,mem_buf_chain_t *chain,
    rtmp_live_codec_t *codec)
{
    uint32_t    profile_idc, width, height, crop_left, crop_right,
                crop_top, crop_bottom, frame_mbs_only, n, cf_idc,
                num_ref_frames;
    mem_buf_t  *buf;
    mem_bits_t  bitstream;


    buf = rtmp_copy_chains_to_temp_buf(chain,session->temp_pool);
    if (buf == NULL) {
        return RTMP_FAILED;
    }

    mem_bits_init(&bitstream,buf);
    mem_bits_read(&bitstream,40);

    codec->avc_version = mem_bits_read(&bitstream,8);
    codec->avc_profile = mem_bits_read(&bitstream,8);
    codec->avc_compat  = mem_bits_read(&bitstream,8);
    codec->avc_level   = mem_bits_read(&bitstream,8);

    /* nal bytes */
    codec->avc_nal_bytes = (mem_bits_read(&bitstream,8) & 0x03) + 1;

    /* nnals */
    if ((mem_bits_read(&bitstream,8) & 0x1f) == 0) {
        return RTMP_FAILED;
    }

    /* nal size */
    mem_bits_read(&bitstream,16);

    /* nal type */
    if (mem_bits_read(&bitstream,8) != 0x67) {
        return RTMP_FAILED;
    }

    /* SPS */

    /* profile idc */
    profile_idc = mem_bits_read(&bitstream,8);

    /* flags */
    mem_bits_read(&bitstream,8);

    /* level idc */
    mem_bits_read(&bitstream,8);

    /* SPS id */
    mem_bits_read_golomb(&bitstream);

    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
        profile_idc == 83 || profile_idc == 86 || profile_idc == 118)
    {
        /* chroma format idc */
        cf_idc = mem_bits_read_golomb(&bitstream);
        
        if (cf_idc == 3) {

            /* separate color plane */
            mem_bits_read(&bitstream,1);
        }

        /* bit depth luma - 8 */
        mem_bits_read_golomb(&bitstream);

        /* bit depth chroma - 8 */
        mem_bits_read_golomb(&bitstream);

        /* qpprime y zero transform bypass */
        mem_bits_read(&bitstream,1);

        /* seq scaling matrix present */
        if (mem_bits_read(&bitstream,1)) {
            if (cf_idc != 3) {
                mem_bits_read(&bitstream,8);
            } else {
                mem_bits_read(&bitstream,12);
            }
        }
    }

    /* log2 max frame num */
    mem_bits_read_golomb(&bitstream);

    /* pic order cnt type */
    switch (mem_bits_read_golomb(&bitstream)) {
    case 0:

        /* max pic order cnt */
        mem_bits_read_golomb(&bitstream);
        break;

    case 1:

        /* delta pic order alwys zero */
        mem_bits_read(&bitstream,1);

        /* offset for non-ref pic */
        mem_bits_read_golomb(&bitstream);

        /* offset for top to bottom field */
        mem_bits_read_golomb(&bitstream);

        /* num ref frames in pic order */
        num_ref_frames = mem_bits_read_golomb(&bitstream);

        for (n = 0; n < num_ref_frames; n++) {

            /* offset for ref frame */
            mem_bits_read_golomb(&bitstream);
        }
    }

    /* num ref frames */
    codec->avc_ref_frames = mem_bits_read_golomb(&bitstream);

    /* gaps in frame num allowed */
    mem_bits_read(&bitstream,1);

    /* pic width in mbs - 1 */
    width = mem_bits_read_golomb(&bitstream);

    /* pic height in map units - 1 */
    height = mem_bits_read_golomb(&bitstream);

    /* frame mbs only flag */
    frame_mbs_only = mem_bits_read(&bitstream,1);

    if (!frame_mbs_only) {

        /* mbs adaprive frame field */
        mem_bits_read(&bitstream,1);
    }

    /* direct 8x8 inference flag */
    mem_bits_read(&bitstream,1);

    /* frame cropping */
    if (mem_bits_read(&bitstream,1)) {

        crop_left = mem_bits_read_golomb(&bitstream);
        crop_right = mem_bits_read_golomb(&bitstream);
        crop_top = mem_bits_read_golomb(&bitstream);
        crop_bottom = mem_bits_read_golomb(&bitstream);

    } else {

        crop_left = 0;
        crop_right = 0;
        crop_top = 0;
        crop_bottom = 0;
    }

    codec->width = (width + 1) * 16 - (crop_left + crop_right) * 2;
    codec->height = (2 - frame_mbs_only) * (height + 1) * 16 -
                  (crop_top + crop_bottom) * 2;

    rtmp_log(RTMP_LOG_DEBUG,
        "codec: avc header profile=%ui, compat=%ui, level=%ui, "
        "nal_bytes=%ui, ref_frames=%ui, width=%ui, height=%ui",
        codec->avc_profile, codec->avc_compat, codec->avc_level,
        codec->avc_nal_bytes, codec->avc_ref_frames,
        codec->width, codec->height);

    return RTMP_OK;
}
