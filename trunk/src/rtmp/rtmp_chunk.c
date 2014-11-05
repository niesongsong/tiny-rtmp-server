
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

uint8_t* rtmp_chunk_read(mem_buf_t *buf,rtmp_chunk_header_t *h)
{
    uint32_t                csid;
    uint8_t                 *p,*last;

    memset(h,0,sizeof(rtmp_chunk_header_t));

    p = buf->buf;
    last = buf->last;

    if (last ==  p) {
        return 0;
    }

    /*Basic Header*/

    h->fmt  = (*p >> 6) & 0x03;
    csid = *p++ & 0x3f;

    if (csid == 0) {

        if (last - p < 1) {
            return 0;
        }
        csid = 64;
        csid += *(uint8_t*)p++;

    } else if (csid == 1){

        if (last - p < 2) {
            return 0;
        }
        csid = 64;
        csid += *p++;
        csid += (uint32_t)*p++ << 8; /*p * 256*/
    }
    h->csid = csid;

    /*Message Header*/

    switch (h->fmt) {

    case 0:

        if (last - p < 11) {
            return 0;
        }

        h->dtime = byte_make_ulong3(p); 
        p += 3;

        h->msglen = byte_make_ulong3(p);
        p += 3;

        h->msgtid = *p++;
        h->msgsid = byte_make_ulong4_l(p);
        p += 4;

        break;

    case 1:

        if (last - p < 7) {
            return 0;
        }

        h->dtime = byte_make_ulong3(p);
        p += 3;

        h->msglen = byte_make_ulong3(p);
        p += 3;

        h->msgtid = *p++;

        break;

    case 2:

        if (last - p < 3) {
            return 0;
        }

        h->dtime = byte_make_ulong3(p);
        p += 3;

        break;
    }

    /*Extern Timestamp*/

    if (h->dtime == 0x00ffffff) {

        if (last - p < 4) {
            return 0;
        }

        h->extend = byte_make_ulong4(p);
        p += 4;
    }

    return p;
}

int32_t rtmp_chunk_write(mem_buf_t *buf,rtmp_chunk_header_t *h)
{
    uint32_t                csid;
    uint8_t                *p,*end;

    p = buf->buf;
    end = buf->end;

    if (end ==  p) {
        return -1;
    }

    /*Basic Header*/

    *p = (h->fmt << 6) & 0xc0;
    csid = h->csid;

    if ((csid < 2) || (csid > 65566)) {
        rtmp_log(RTMP_LOG_ERR,"invalid csid: [%d]",csid);
        return -1;
    }

    if (csid <= 63) {

        *p++ |= (uint8_t)csid;

    } else if (csid <= 319) {

        if (end - p < 1) {
            return -1;
        }

        *p++ &= 0xc0;
        *p++ = (csid - 64);

    } else {

        if (end - p < 2) {
            return -1;
        }

        csid -= 64;
        *p++ |= 0x3f;

        *p++ = csid & 0xff;
        *p++ = csid >> 8;
    }

    /*Message Header*/

    switch (h->fmt) {

    case 0:

        if (end - p < 11) {
            return -1;
        }

        ulong_make_byte3(p,h->dtime); 
        p += 3;

        ulong_make_byte3(p,h->msglen);
        p += 3;

        *p++ = h->msgtid;

        ulong_make_byte4_l(p,h->msgsid);
        p += 4;

        break;

    case 1:

        if (end - p < 7) {
            return -1;
        }

        ulong_make_byte3(p,h->dtime);
        p += 3;

        ulong_make_byte3(p,h->msglen);
        p += 3;

        *p++ = h->msgtid;

        break;

    case 2:

        if (end - p < 3) {
            return -1;
        }

        ulong_make_byte3(p,h->dtime);
        p += 3;

        break;
    }

    /*Extern Timestamp*/

    if (h->dtime == 0x00ffffff) {

        if (end - p < 4) {
            return -1;
        }

        ulong_make_byte4_l(p,h->extend);
        p += 4;
    }

    buf->last = p;

    return 0;
}
