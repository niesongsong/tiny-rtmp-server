/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __BYTE_ORDER_H_INCLUDED__
#define __BYTE_ORDER_H_INCLUDED__

#define BYTE_LITTLE_ENDIAN   0
#define BYTE_BIG_ENDIAN      1

#ifdef HAVE_USING_LEDIAN
#define BYTE_ENDIAN     BYTE_LITTLE_ENDIAN
#else
#define BYTE_ENDIAN     BYTE_BIG_ENDIAN 
#endif

#define byte_read(in,out,l)     byte_write(in,out,l,BYTE_ENDIAN)

#define byte_read_int8(in,out)     byte_write(in,out,1,BYTE_ENDIAN)
#define byte_read_int16(in,out)    byte_write(in,out,2,BYTE_ENDIAN)
#define byte_read_int32(in,out)    byte_write(in,out,4,BYTE_ENDIAN)
#define byte_read_int64(in,out)    byte_write(in,out,8,BYTE_ENDIAN)

#define byte_write_int8(in,out)    byte_write(in,out,1,BYTE_ENDIAN)
#define byte_write_int16(in,out)   byte_write(in,out,2,BYTE_ENDIAN)
#define byte_write_int32(in,out)   byte_write(in,out,4,BYTE_ENDIAN)
#define byte_write_int64(in,out)   byte_write(in,out,8,BYTE_ENDIAN)

int byte_write(const char *in,char *out,int l,int order);
void byte_fill_random(char *buf,int size);

#endif
