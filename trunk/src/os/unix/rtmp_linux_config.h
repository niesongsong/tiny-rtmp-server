
/*
 * Copyright (C) nie950@gmail.com
 */


#ifndef __LINUX_H_INCLUDED__
#define __LINUX_H_INCLUDED__

#define HAVE_OS_LINUX

#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

#define SOCK_EAGAIN         EAGAIN
#define SOCK_ENFILE         ENFILE
#define SOCK_EMFILE         EMFILE

#ifndef inline
#define inline __inline
#endif

#define rtmp_socket(af, type, proto)  socket(af,type,proto)

typedef int   fd_t;
typedef int   socket_t;

#define sock_errno         errno
#define closesocket(c)     close(c)
#define set_nonblocking(s) fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define set_blocking(s)    fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)

#endif