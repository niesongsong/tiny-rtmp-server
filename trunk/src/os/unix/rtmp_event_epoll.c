
/*
 * Copyright (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

#ifdef HAVE_EPOLL_STUB 

#define EPOLLIN        0x001
#define EPOLLPRI       0x002
#define EPOLLOUT       0x004
#define EPOLLRDNORM    0x040
#define EPOLLRDBAND    0x080
#define EPOLLWRNORM    0x100
#define EPOLLWRBAND    0x200
#define EPOLLMSG       0x400
#define EPOLLERR       0x008
#define EPOLLHUP       0x010

#define EPOLLET        0x80000000
#define EPOLLONESHOT   0x40000000

#define EPOLL_CTL_ADD  1
#define EPOLL_CTL_DEL  2
#define EPOLL_CTL_MOD  3

typedef union epoll_data {
    void         *ptr;
    int           fd;
    uint32_t      u32;
    uint64_t      u64;
} epoll_data_t;

struct epoll_event {
    uint32_t      events;
    epoll_data_t  data;
};

int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int nevents, int timeout);

int epoll_create(int size)
{
    return -1;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    return -1;
}

int epoll_wait(int epfd, struct epoll_event *events, int nevents, int timeout)
{
    return -1;
}

#endif

typedef struct event_io_epoll_ctx_s event_io_epoll_ctx_t;
struct event_io_epoll_ctx_s {
    int                  efd;
    uint32_t             nevents;
    struct epoll_event   event_list[1];
};

static int32_t event_io_epoll_init(rtmp_event_io_t *io);
static int32_t event_io_epoll_add(rtmp_event_io_t *io,
    rtmp_event_t *ev,uint32_t flag);
static int32_t event_io_epoll_add_conn(rtmp_event_io_t *io,
    rtmp_connection_t *c);
static int32_t event_io_epoll_poll(rtmp_event_io_t *io,
    uint32_t msec,uint32_t flag);
static int32_t event_io_epoll_del(rtmp_event_io_t *io,
    rtmp_event_t *ev,uint32_t flag);
static int32_t event_io_epoll_del_conn(rtmp_event_io_t *io,
    rtmp_connection_t *c);
static int32_t event_io_epoll_done(rtmp_event_io_t *io);

int32_t event_io_accept_disable(rtmp_cycle_t *cycle);
int32_t event_io_accept_enable(rtmp_cycle_t *cycle);

rtmp_event_io_t event_io_epoll = {
    EVENT_IO_EPOLL,
    "epoll",
    (void*)0,
    0,
    event_io_epoll_init,
    event_io_epoll_add,
    event_io_epoll_add_conn,
    event_io_epoll_poll,
    event_io_epoll_del,
    event_io_epoll_del_conn,
    event_io_epoll_done,
    {0,0},
    {0,0}
};

int32_t event_io_epoll_init(rtmp_event_io_t *io)
{
    event_io_epoll_ctx_t *ectx;
    uint32_t              nevents;

    nevents = io->max_conn/2;

    ectx = mem_calloc(sizeof(event_io_epoll_ctx_t) + 
        sizeof(struct epoll_event) * nevents);
    if (ectx == NULL) {
        return RTMP_FAILED;
    }

    ectx->efd = epoll_create(nevents);
    if (io->data) {
        rtmp_log(RTMP_LOG_WARNING,"free ectx");
        mem_free(ectx);
        io->data = 0;
    }
    
    if (ectx->efd == -1) {
        mem_free(ectx);
        return RTMP_FAILED;
    }

    ectx->nevents = nevents;
    io->data = ectx;

    return RTMP_OK;
}

static int32_t event_io_epoll_add_conn(rtmp_event_io_t *io,
    rtmp_connection_t *c)
{
    struct epoll_event ee;
    event_io_epoll_ctx_t *ectx;
    
    ectx = io->data;
    if (ectx == NULL) {
        return RTMP_FAILED;
    }

    ee.events = EPOLLIN|EPOLLOUT|EPOLLET;
    ee.data.ptr = (void *)c;

    if (epoll_ctl(ectx->efd,EPOLL_CTL_ADD,c->fd,&ee) == -1) {

        rtmp_log(RTMP_ERROR,"epoll_ctl() failed %d",c->fd);
        return RTMP_FAILED;
    }

    c->read->active = 1;
    c->write->active = 1;

    return RTMP_OK;
}

static int32_t event_io_epoll_del_conn(rtmp_event_io_t *io,
    rtmp_connection_t *c)
{
    struct epoll_event ee;
    event_io_epoll_ctx_t *ectx;

    ectx = io->data;
    if (ectx == NULL) {
        return RTMP_FAILED;
    }

    if (epoll_ctl(ectx->efd, EPOLL_CTL_DEL, c->fd, &ee) == -1) {
        rtmp_log(RTMP_ERROR,"epoll_ctl() failed %d",c->fd);
        return RTMP_FAILED;
    }

    c->read->active = 0;
    c->write->active = 0;

    return RTMP_OK;
}

int32_t event_io_epoll_add(rtmp_event_io_t *io,rtmp_event_t *ev,uint32_t flag)
{
    struct epoll_event    ee;
    event_io_epoll_ctx_t *ectx;
    rtmp_connection_t    *c;
    rtmp_event_t         *ep;
    int                   prev,mode;

    ectx = io->data;
    c = ev->data;
    ep = NULL;

    if (ectx == NULL) {
        return RTMP_FAILED;
    }

    if ((flag != EVENT_READ) && (flag != EVENT_WRITE)) {
        rtmp_log(RTMP_LOG_WARNING,"unknown flag %d",flag);
        return RTMP_FAILED;
    }

    if (ev->active == 1) {
        rtmp_log(RTMP_LOG_WARNING,"added already %d",c);
        return RTMP_OK;
    }

    if (flag == EVENT_READ) {
        ep = c->write;
        prev = EPOLLOUT;
    }

    if (flag == EVENT_WRITE) {
        ep = c->read;
        prev = EPOLLIN;
    }

    if (ep->active) {
        mode = EPOLL_CTL_MOD;
        ee.events = prev | EPOLLET;
        ee.data.ptr = c;

    } else {
        mode = EPOLL_CTL_ADD;
        ee.events = EPOLLIN | EPOLLOUT | EPOLLET;
        ee.data.ptr = c;
    }

    if (epoll_ctl(ectx->efd, mode , c->fd, &ee) == -1) {

        rtmp_log(RTMP_LOG_ERR, "epoll_ctl(%d, %d) failed",
            mode, c->fd);

        return RTMP_FAILED;
    }

    ev->active = 1;

    return RTMP_OK;
}

int32_t event_io_epoll_poll(rtmp_event_io_t *io, uint32_t msec, uint32_t flag)
{
    event_io_epoll_ctx_t    *ectx;
    rtmp_event_t            *rev,*wev;
    rtmp_connection_t       *c;
    queue_t                 *queue;
    uint32_t                 revents;
    int                      i,events,err;

    ectx = io->data;
    if (ectx == NULL) {
        return RTMP_FAILED;
    }

    events = epoll_wait(ectx->efd, ectx->event_list, 
        (int) ectx->nevents, msec);

    err = (events == -1) ? sock_errno : 0;

    if (flag & EVENT_UPDATE) {
        rtmp_time_update();
    }

    if (err) {
        if (err == EINTR) {
            return RTMP_OK;
        }

        return RTMP_FAILED;
    }

    if (events == 0) {
        if (msec != (uint32_t)-1) {
            return RTMP_OK;
        }
        rtmp_log(RTMP_LOG_WARNING,"epoll_wait() returned no events "
                 "without timeout");

        return RTMP_FAILED;
    }

    for (i = 0;i < events; i++) {

        c = ectx->event_list[i].data.ptr;
        if (c->fd == -1) {            
            continue;
        }

        revents = ectx->event_list[i].events;
        rev = c->read;

        if ((revents & (EPOLLERR|EPOLLHUP))
            && (revents & (EPOLLIN|EPOLLOUT)) == 0)
        {
            revents |= EPOLLIN|EPOLLOUT;
        }

        if ((revents & EPOLLIN) && rev->active) {
            rev->ready = 1;

            if (rev->accept) {
                queue = (queue_t *)& io->accept;

            } else {
                queue = (queue_t *)& io->posted;

            }
            queue_push(queue,& rev->link);
        }

        wev = c->write;

        if ((revents & EPOLLOUT) && wev->active) {
            wev->ready = 1;
            queue_push(& io->posted,& wev->link);
        }

    }

    return RTMP_OK;
}

int32_t event_io_epoll_del(rtmp_event_io_t *io,rtmp_event_t *ev,uint32_t flag)
{
    struct epoll_event    ee;
    event_io_epoll_ctx_t *ectx;
    rtmp_connection_t    *c;
    rtmp_event_t         *ep;
    int                   prev,mode;

    ectx = io->data;
    c = ev->data;
    ep = NULL;

    if (ectx == NULL) {
        return RTMP_FAILED;
    }

    if ((flag != EVENT_READ) && (flag != EVENT_WRITE)) {
        rtmp_log(RTMP_LOG_WARNING,"unknown flag %d",flag);
        return RTMP_FAILED;
    }

    if (ev->active == 0) {
        rtmp_log(RTMP_LOG_WARNING,"delete already %d",c);
        return RTMP_OK;
    }

    if (flag == EVENT_READ) {
        ep = c->write;
        prev = EPOLLOUT;
    }

    if (flag == EVENT_WRITE) {
        ep = c->read;
        prev = EPOLLIN;
    }

    if (ep->active) {
        mode = EPOLL_CTL_MOD;
        ee.events = prev | EPOLLET;
        ee.data.ptr = c;

    } else {
        mode = EPOLL_CTL_DEL;
        ee.events = 0;
        ee.data.ptr = NULL;
    }

    if (epoll_ctl(ectx->efd, mode , c->fd, &ee) == -1) {
        rtmp_log(RTMP_LOG_ERR, "epoll_ctl(%d, %d) failed",
            mode, c->fd);

        return RTMP_FAILED;
    }

    ev->active = 0;

    return RTMP_OK;
}

int32_t event_io_epoll_done(rtmp_event_io_t *io)
{
    event_io_epoll_ctx_t   *ectx;

    ectx = io->data;
    if (ectx) {
        io->data = 0;

        if (close(ectx->efd) == -1) {
            rtmp_log(RTMP_LOG_WARNING,"epoll_close failed[%d]",ectx->efd);
        }

        mem_free(ectx);
    }

    return RTMP_OK;
}
