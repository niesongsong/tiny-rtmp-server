
/*
 * Copyright (C) nie950@gmail.com
 */


#ifndef __WIN32_H_INCLUDED__
#define __WIN32_H_INCLUDED__

#define _CRT_SECURE_NO_WARNINGS

#include <winsock.h>
#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define HAVE_USING_LEDIAN   1
#define HAVE_OS_WIN32       1
#define HAVE_EPOLL_STUB     1

#define SOCK_EAGAIN         WSAEWOULDBLOCK
#define SOCK_ENFILE         WSAEMFILE
#define SOCK_EMFILE         WSAEMFILE
#define SOCK_ERROR          (-1)
#define SOCK_OK             (0)

#ifndef inline
#define inline __inline
#endif

typedef DWORD   pid_t;
typedef DWORD   fd_t;

typedef SOCKET  socket_t;
typedef int     socklen_t;

#define rtmp_socket(af,type,proto)  socket(af,type,proto)
#define fork() 1

#define sock_errno WSAGetLastError()

#define msleep(x)  Sleep(x)

#define sched_yield()   SwitchToThread()

int set_nonblocking(socket_t s);
int set_blocking(socket_t s);
int set_tcppush(socket_t s);

#endif