/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


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


void byte_fill_random(char *buf,int size)
{
    while (--size > 0) {
        buf[size] = (unsigned char)rand();
    }
}