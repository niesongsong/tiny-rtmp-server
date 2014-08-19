
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __ERROR_H_INCLUDED__
#define __ERROR_H_INCLUDED__


#define RTMP_NULL           ((void*)0)
#define RTMP_READY          ((void*)-1)

#define RTMP_OK             (0)
#define RTMP_ERROR          (-1)
#define RTMP_FAILED         (-1)
#define RTMP_IGNORE         (-2)
#define RTMP_ERROR_MAX      (32)

int32_t rtmp_strerror_init(void );
char* rtmp_strerror(int32_t err, char *errstr, size_t size);

#endif
