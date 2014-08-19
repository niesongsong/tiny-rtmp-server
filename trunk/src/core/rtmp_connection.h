
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __LISTENING_H_INCLUDED__
#define __LISTENING_H_INCLUDED__

struct rtmp_listening_s {
    socket_t                fd;
    
    struct sockaddr         sockaddr;
    socklen_t               socklen;
    char                    sockaddr_text[32];
    connection_handler_pt   handler;

    rtmp_connection_t      *connection;
    rtmp_cycle_t           *cycle;

    void                   *data;
};


struct rtmp_connection_s {
    void               *data;

    fd_t                fd;
    rtmp_event_t       *read;
    rtmp_event_t       *write;
    rtmp_listening_t   *listening;
    rtmp_connection_t  *next;

    /*remote*/
    struct sockaddr     sockaddr;
    socklen_t           socklen;
    char                addr_text[32];

    /*local*/
    struct sockaddr     local_sockaddr;
    socklen_t           local_socklen;
    char                local_addr_text[32];

    mem_pool_t         *pool;
};

rtmp_connection_t *get_connection(rtmp_listening_t *ls,fd_t s);
void free_connection(rtmp_connection_t *c);
void close_connection(rtmp_connection_t *c);

/*[00][00][0000]:  family:port:addr*/
int32_t sockaddr_sin_cmp(struct sockaddr *sin1,struct sockaddr *sin2);

#endif