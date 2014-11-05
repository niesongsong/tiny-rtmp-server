
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

#ifndef __CHUNK_H_INCLUDED__
#define __CHUNK_H_INCLUDED__

#define RTMP_DEFAULT_IN_CHUNKSIZE   128
#define RTMP_DEFAULT_OUT_CHUNKSIZE  4096
#define RTMP_MAX_CHUNK_HEADER       18

/*
    +--------------+----------------+--------------------+--------------+
    | Basic Header | Message Header | Extended Timestamp |  Chunk Data  |
    +--------------+----------------+--------------------+--------------+
    |                                                    |
    |<------------------- Chunk Header ----------------->|


    Basic Header:  (1,2,3 Byte)

         0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |fmt|    cs id  |
        +-+-+-+-+-+-+-+-+

    Message Header: (11,7,3,0 Byte)

         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                    timestamp                  |message length |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |     message length (cont)     |message type id| msg stream id |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |            message stream id (cont)           |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


    Extended Timestamp: (4,0 Byte)

         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                    timestamp                  |message length |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct rtmp_chunk_header_s rtmp_chunk_header_t;
typedef struct rtmp_chunk_stream_s rtmp_chunk_stream_t;

/*chunk header*/
struct rtmp_chunk_header_s{
    uint32_t        fmt:2;
    uint32_t        csid:24;
    
    uint32_t        dtime:24; /*timestamp (delta)*/
    uint32_t        msglen:24;
    uint32_t        msgtid:8;
    uint32_t        msgsid;

    uint32_t        extend;
    uint32_t        chunk_time;
};

struct rtmp_chunk_stream_s {
    rtmp_chunk_header_t   hdr;
    uint32_t              recvlen; 
    mem_buf_chain_t      *chain;
    mem_buf_chain_t      *last;
};

uint8_t* rtmp_chunk_read(mem_buf_t *buf,rtmp_chunk_header_t *h);
int32_t rtmp_chunk_write(mem_buf_t *buf,rtmp_chunk_header_t *h);

#endif

