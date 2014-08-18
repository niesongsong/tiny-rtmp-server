
/*
 * Copyright (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

typedef struct event_io_select_ctx_s event_io_select_ctx_t;
struct event_io_select_ctx_s {
    fd_set          read_set;
    fd_set          write_set;

    fd_set          saved_read;
    uint32_t        max_read;

    fd_set          saved_write;
    uint32_t        max_write;

    rtmp_event_t   *event_list[1];
};

static int32_t event_io_select_init(rtmp_event_io_t *io);
static int32_t event_io_select_add(rtmp_event_io_t *io,
    rtmp_event_t *ev,uint32_t flag);
static int32_t event_io_select_poll(rtmp_event_io_t *io,
    uint32_t msec,uint32_t flag);
static int32_t event_io_select_del(rtmp_event_io_t *io,
    rtmp_event_t *ev,uint32_t flag);
static int32_t event_io_select_done(rtmp_event_io_t *io);

static int32_t event_io_select_add_conn(rtmp_event_io_t *io,
    rtmp_connection_t *c);
static int32_t event_io_select_del_conn(rtmp_event_io_t *io,
    rtmp_connection_t *c);

int32_t event_io_accept_disable(rtmp_cycle_t *cycle);
int32_t event_io_accept_enable(rtmp_cycle_t *cycle);

static int32_t event_select_repair_fd_sets(rtmp_event_io_t *io);

rtmp_event_io_t event_io_select = {
    EVENT_IO_SELECT,
    "select",
    0,
    0,
    event_io_select_init,
    event_io_select_add,
    event_io_select_add_conn,
    event_io_select_poll,
    event_io_select_del,
    event_io_select_del_conn,
    event_io_select_done
};

int32_t event_io_select_init(rtmp_event_io_t *io)
{
    event_io_select_ctx_t *sctx;

    sctx = mem_calloc(sizeof(event_io_select_ctx_t) + 
        io->max_conn * 2 * sizeof(void*));

    if (sctx == NULL) {
        return RTMP_FAILED;
    }

    if (io->data) {

        rtmp_log(RTMP_LOG_WARNING,"free sctx");
        mem_free(sctx);

        io->data = 0;
    }

    io->data = sctx;

    FD_ZERO(& sctx->read_set);
    FD_ZERO(& sctx->write_set);
    FD_ZERO(& sctx->saved_read);
    FD_ZERO(& sctx->saved_write);

    return RTMP_OK;
}

int32_t event_io_select_add(rtmp_event_io_t *io,rtmp_event_t *ev,uint32_t flag)
{
    rtmp_connection_t       *c;
    event_io_select_ctx_t   *sctx;
    uint32_t                 nevents;

    if (ev->index != -1) {
        rtmp_log(RTMP_LOG_WARNING,"event already!");
        return RTMP_FAILED;
    }

    c = ev->data;

    sctx = io->data;
    if (sctx == NULL) {
        return RTMP_FAILED;
    }

    if ((flag != EVENT_READ) && (flag != EVENT_WRITE)) {
        rtmp_log(RTMP_LOG_WARNING,"unknown flag %d",flag);
        return RTMP_FAILED;
    }

    if (flag == EVENT_READ) {
        
        if (ev->write) {
            rtmp_log(RTMP_LOG_ERR,"read event");
            return RTMP_FAILED;
        }

        if (sctx->max_read >= FD_SETSIZE) {
            rtmp_log(RTMP_LOG_ERR,"too many read events");
            return RTMP_FAILED;
        }

        FD_SET(c->fd, &sctx->saved_read);
        sctx->max_read++;
    }

    if (flag == EVENT_WRITE) {
        
        if (!ev->write) {
            rtmp_log(RTMP_LOG_ERR,"write event");
            return RTMP_FAILED;
        }

        if (sctx->max_write >= FD_SETSIZE) {
            rtmp_log(RTMP_LOG_ERR,"too many write events");
            return RTMP_FAILED;
        }

        FD_SET(c->fd, &sctx->saved_write);
        sctx->max_write++;
    }
    
    nevents = sctx->max_read + sctx->max_write - 1;

    ev->active = 1;
    sctx->event_list[nevents] = ev;
    ev->index = nevents;

    return RTMP_OK;
}

int32_t event_io_select_poll(rtmp_event_io_t *io, uint32_t msec, uint32_t flag)
{
    struct timeval          *tp,tv;
    event_io_select_ctx_t   *sctx;
    rtmp_event_t            *ev;
    rtmp_connection_t       *c;
    queue_t                 *queue;
    uint32_t                 i,nevents;
    int                      ready, nready,found;
    
    if (msec != (uint32_t)-1) {
        tv.tv_sec = (long) (msec / 1000);
        tv.tv_usec = (long) ((msec % 1000) * 1000);
        tp = &tv;
    } else {
        tp = NULL;
    }

    sctx = io->data;
    if (sctx == NULL) {
        return RTMP_FAILED;
    }

    sctx->read_set = sctx->saved_read;
    sctx->write_set = sctx->saved_write;

    if (sctx->max_write || sctx->max_read) {
        ready = select(0, &sctx->read_set, &sctx->write_set, NULL, tp);

    } else {
        msleep(msec);
        ready = 0;

    }
    
    if (flag & EVENT_UPDATE) {
        rtmp_time_update();
    }

    if (ready <= 0) {
        if (sock_errno == WSAENOTSOCK) {
            event_select_repair_fd_sets(io);
        }

        return RTMP_FAILED;
    }

    nready = 0;
    nevents = sctx->max_write + sctx->max_read;

    for (i = 0; i < nevents; i++) {
        ev = sctx->event_list[i];
        c = ev->data;
        found = 0;

        ev->ready = 0;

        if (ev->write) {
            if (FD_ISSET(c->fd, &sctx->write_set)) {
                found = 1;
            }

        } else {
            if (FD_ISSET(c->fd, &sctx->read_set)) {
                found = 1;
            }
        }

        if (found) {
            ev->ready = 1;

            if (ev->accept) {
                queue = (queue_t *)& io->accept;
                
            } else {
                queue = (queue_t *)& io->posted;

            }
            queue_push(queue,& ev->link);

            nready++;
        }
    }

    if (ready != nready) {
        event_select_repair_fd_sets(io);
    }

    return RTMP_OK;
}

int32_t event_io_select_del(rtmp_event_io_t *io,rtmp_event_t *ev,uint32_t flag)
{
    rtmp_connection_t       *c;
    event_io_select_ctx_t   *sctx;
    rtmp_event_t            *e;
    uint32_t                 nevents;

    if (ev->index == -1) {
        rtmp_log(RTMP_LOG_WARNING,"event already!");
        return RTMP_FAILED;
    }

    sctx = io->data;
    if (sctx == NULL) {
        return RTMP_FAILED;
    }

    c = ev->data;
    ev->active = 0;

    if ((flag != EVENT_READ) && (flag != EVENT_WRITE)) {
        rtmp_log(RTMP_LOG_WARNING,"unknown flag %d",flag);
        return RTMP_FAILED;
    }

    if (flag == EVENT_READ) {
        FD_CLR(c->fd, &sctx->saved_read);
        sctx->max_read--;
    } 
    
    if (flag == EVENT_WRITE) {
        FD_CLR(c->fd, &sctx->saved_write);
        sctx->max_write--;
    }

    nevents = sctx->max_write + sctx->max_read;

    if (ev->index < (int32_t)nevents) {
        e = sctx->event_list[nevents];

        sctx->event_list[ev->index] = e;
        e->index = ev->index;
    }

    ev->index = -1;

    return RTMP_OK;
}

int32_t event_io_select_done(rtmp_event_io_t *io)
{
    event_io_select_ctx_t   *sctx;

    sctx = io->data;
    if (sctx) {
        io->data = 0;
        mem_free(sctx);
    }

    return RTMP_OK;
}

int32_t event_select_repair_fd_sets(rtmp_event_io_t *io)
{
    u_int                   i;
    int                     n;
    socklen_t               len;
    socket_t                s;
    event_io_select_ctx_t   *sctx;

    sctx = io->data;

    for (i = 0; i < sctx->saved_read.fd_count; i++) {

        s = sctx->saved_read.fd_array[i];
        len = sizeof(int);

        if (getsockopt(s, SOL_SOCKET, SO_TYPE, (char *) &n, &len) == -1) {

            rtmp_log(RTMP_LOG_NORMAL, "invalid descriptor %d "
                "in read fd_set,err[%d]", s,sock_errno);

            FD_CLR(s, &sctx->saved_read);
        }
    }

    for (i = 0; i < sctx->saved_write.fd_count; i++) {

        s = sctx->saved_write.fd_array[i];
        len = sizeof(int);

        if (getsockopt(s, SOL_SOCKET, SO_TYPE, (char *) &n, &len) == -1) {

            rtmp_log(RTMP_LOG_NORMAL, "invalid descriptor %d "
                "in write fd_set,err[%d]", s,sock_errno);

            FD_CLR(s, &sctx->saved_write);
        }
    }

    return RTMP_OK;
}

static int32_t event_io_select_add_conn(rtmp_event_io_t *io,
    rtmp_connection_t *c)
{
    return event_io_select_add(io,c->read,EVENT_READ);
}

static int32_t event_io_select_del_conn(rtmp_event_io_t *io,
    rtmp_connection_t *c)
{
    if (c->write->active) {
        rtmp_event_delete(c->write,EVENT_WRITE);
    }

    if (c->read->active) {
        rtmp_event_delete(c->read,EVENT_READ);
    }

    return RTMP_OK;
}
