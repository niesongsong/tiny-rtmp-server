
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


void drain_connections()
{
    return ;
}

rtmp_connection_t *get_connection(rtmp_listening_t  *ls,fd_t s)
{
    rtmp_connection_t  *c;
    rtmp_event_t       *rev, *wev;
    mem_pool_t         *pool;
    rtmp_cycle_t       *rtmp_cycle;

    rtmp_cycle = ls->cycle;

    c = rtmp_cycle->free_connections;

    if (c == NULL) {
        drain_connections();
        c = rtmp_cycle->free_connections;
    }

    if (c == NULL) {
        rtmp_log(RTMP_LOG_DEBUG,"no enough connections[%d]!",s);
        return NULL;
    }

    rtmp_cycle->free_connections = c->next;
    rev = c->read;
    wev = c->write;
    pool = c->pool;

    memset(c, 0, sizeof(rtmp_connection_t));

    c->read = rev;
    c->write = wev;
    c->fd = s;
    c->pool = pool;
    
    memset(rev, 0 ,sizeof(rtmp_event_t));
    memset(wev, 0, sizeof(rtmp_event_t));

    rev->data = c;
    wev->data = c;
    wev->write = 1;

    rev->index = -1;
    wev->index = -1;

    rtmp_log(RTMP_LOG_DEBUG,"get connection:%p",c);

    return c;
}

void close_connection(rtmp_connection_t *c)
{
    rtmp_cycle_t  *ngx_cycle;
    fd_t fd;

    ngx_cycle = c->listening->cycle;

    fd = c->fd;
    if (fd == -1) {
        rtmp_log(RTMP_LOG_DEBUG,"[%d]already closed socket!",c->fd);
        return ;
    }

    if (c->read->timer_set) {
        rtmp_event_del_timer(c->read);
    }

    if (c->write->timer_set) {
        rtmp_event_del_timer(c->write);
    }

    rtmp_event_delete_conn(c);

    c->fd = -1;
    closesocket(fd);
    
    rtmp_log(RTMP_LOG_DEBUG,"[%d]closed socket",fd);

    c->next = ngx_cycle->free_connections;
    ngx_cycle->free_connections = c;

    rtmp_log(RTMP_LOG_DEBUG,"close connection[%p]",c);

    return ;
}

void free_connection(rtmp_connection_t *c)
{
    rtmp_cycle_t  *ngx_cycle;
    fd_t fd;
    
    ngx_cycle = c->listening->cycle;

    fd = c->fd;
    c->fd = -1;

    if (fd != -1) {
        rtmp_log(RTMP_LOG_DEBUG,"close socket",fd);
        closesocket(fd);
    }

    c->next = ngx_cycle->free_connections;
    ngx_cycle->free_connections = c;

    rtmp_log(RTMP_LOG_DEBUG,"free connection:%p",c);

    return ;
}

int32_t sockaddr_sin_cmp(struct sockaddr *sin1,struct sockaddr *sin2)
{
    struct sockaddr_in *s1,*s2;
    int32_t family,port,addr;

    s1 = (struct sockaddr_in *)sin1;
    s2 = (struct sockaddr_in *)sin2;

    family = s1->sin_family - s2->sin_family;
    if (family > 0) {

        family = 0x10;

    } else if (family < 0){

        family = 0x01;
    }

    port = s1->sin_port - s2->sin_port;
    if (port > 0) {

        port = 0x10;

    } else if (port < 0){

        port = 0x01;
    }

    addr = s1->sin_addr.s_addr - s1->sin_addr.s_addr;
    if (addr > 0) {

        addr = 0x10;

    } else if (addr < 0){

        addr = 0x01;
    }

    return (family << 24) + (port << 16) + addr;
}