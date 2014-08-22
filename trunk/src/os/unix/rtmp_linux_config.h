
/*
 * CopyLeft (C) nie950@gmail.com
 */


#ifndef __LINUX_H_INCLUDED__
#define __LINUX_H_INCLUDED__

#define HAVE_OS_LINUX

#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <signal.h>

#define HAVE_USING_LEDIAN   1
#define HAVE_EVENT_EPOLL    1

#define SOCK_EAGAIN         EAGAIN
#define SOCK_ENFILE         ENFILE
#define SOCK_EMFILE         EMFILE
#define SOCK_ERROR          (-1)
#define SOCK_OK             (0)

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
int set_tcppush(socket_t s);

int rtmp_init_signals();

#endif
