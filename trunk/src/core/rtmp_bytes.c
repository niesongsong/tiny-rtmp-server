/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

#if 0

static int byte_write_little(const char *in,char *out,int l);
static int byte_write_big(const char *in,char *out,int l);

int byte_write(const char *in,char *out,int l,int order)
{
    if (order == BYTE_LITTLE_ENDIAN) {
        return byte_write_little(in,out,l);
    } 
    return byte_write_big(in,out,l);
}

int byte_write_little(const char *in,char *out,int l)
{
    char temp[8];

    switch (l) {
    case 1:
        *out = *in;

        break;

    case 2:
        memcpy(temp,in,2);

        out[0] = temp[1];
        out[1] = temp[0];

        break;

    case 4: 
        memcpy(temp,in,4);

        out[0] = temp[3];
        out[1] = temp[2];
        out[2] = temp[1];
        out[3] = temp[0];

        break;

    case 8:
        memcpy(temp,in,8);

        out[0] = temp[7];
        out[1] = temp[6];
        out[2] = temp[5];
        out[3] = temp[4];
        out[4] = temp[3];
        out[5] = temp[2];
        out[6] = temp[1];
        out[7] = temp[0];

        break;

    default:
        return -1;
    }

    return 0;
}

static int byte_write_big(const char *in,char *out,int l)
{
    char temp[8];

    if (in == out) {
        return 0;
    }

    switch (l) {
    case 1:
        *out = *in;

        break;

    case 2: 
    case 4: 
    case 8: 
        memcpy(temp,in,l);
        memcpy(out,temp,l);

        break;

    default:

        return -1;
    }

    return 0;
}

#endif

void byte_fill_random(char *buf,int size)
{
    while (size-- > 0) {
        buf[size] = (unsigned char)rand();
    }
}

uint32_t mem_bits_init(mem_bits_t *b,mem_buf_t *buf)
{
    b->buf = *buf;
    b->offset = 0;

    return RTMP_OK;
}

uint32_t mem_bits_read(mem_bits_t *b,uint16_t l)
{
    uint32_t    n,c,i,j;
    uint8_t     buf[8];

    c = (uint32_t)(b->buf.last - b->buf.buf);
    if ((b->offset + l > c * 8) || (l > 32)) {
        return (uint32_t)-1;
    }

    memcpy(buf,& b->buf.buf[b->offset/8],(l+7)/8);
    j = b->offset % 8;

    n = 0;
    for (i = j;i < j + l;i++) {
        n = (uint32_t)((buf[i/8] >> (7-i%8)) & 1) | (n << 1);
    }
    b->offset += l;

    return n;
}

uint32_t mem_bits_read_golomb(mem_bits_t *b)
{
    uint32_t n;

    for (n = 0;mem_bits_read(b,1) == 0;n++) {
        if (b->offset >= (uint32_t)(b->buf.last - b->buf.buf) * 8) {
            break;
        }
    }

    return (1 << n) + mem_bits_read(b,n) - 1;
}