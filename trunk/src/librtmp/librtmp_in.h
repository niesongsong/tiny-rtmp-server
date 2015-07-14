
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __LIB_RTMP_IN_H_INCLUDED__
#define __LIB_RTMP_IN_H_INCLUDED__

#define HAVE_DEBUG                      1

#ifdef WIN32
#define HAVE_WIN32                      1
#define _CRT_SECURE_NO_WARNINGS
#else
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#if HAVE_WIN32

#include <WinSock2.h>
#include <windows.h>

#define sock_errno         WSAGetLastError()
#define mutex_trylock(x)  (InterlockedCompareExchange((long *)x, 1, 0) == 0)
#define sleep(n)          Sleep((n)*1000)
#define msleep(n)         Sleep(n)
#define EBLOCK            WSAEWOULDBLOCK

typedef SOCKET socket_t;
typedef int socklen_t;

#else

#include <unistd.h>
#define sock_errno        errno

#define closesocket(fd)   close(fd)
#define mutex_trylock(x)  __sync_bool_compare_and_swap((long *)x,0,1)
#define msleep(n)         usleep(n*1000)
#define EBLOCK            EAGAIN

#endif

#define mutex_init(x)       ((*(x))=0)
#define mutex_lock(x)       do {msleep(0);}while(mutex_trylock(x))
#define mutex_unlock(x)     ((*(x))=0)
#define _array_length(a)    (sizeof(a)/sizeof((a)[0]))

#include "librtmp.h"

#include "rtmp_time.h"
#include "rtmp_link.h"
#include "rtmp_log.h"
#include "rtmp_amf.h"
#include "rtmp_bytes.h"
#include "rtmp_alloc.h"

#endif