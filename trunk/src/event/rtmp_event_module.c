
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

extern rtmp_event_io_t event_io_select;

#ifdef HAVE_EVENT_EPOLL
extern rtmp_event_io_t event_io_epoll;
#endif

static void *rtmp_event_create_module(rtmp_cycle_t *cycle);
static int32_t rtmp_event_init_cycle(rtmp_cycle_t *cycle,
    rtmp_module_t *module);
static int32_t rtmp_event_init_process(rtmp_cycle_t *cycle,
    rtmp_module_t *module);
static int32_t rtmp_event_eixt_cycle(rtmp_cycle_t *cycle,
    rtmp_module_t *module);

int32_t event_io_accept_enable(rtmp_cycle_t *cycle);
int32_t event_io_accept_disable(rtmp_cycle_t *cycle);

extern void rtmp_event_accept(rtmp_event_t *ev);

typedef struct rtmp_event_ctx_s {
    uint32_t        max_conn;
    uint32_t        use;

    rtmp_event_io_t event_io;

    rbtree_t        timers;

    atomic_t        accept_mutex;
    uint32_t        accept_mutex_held;
    uint32_t        use_accept_mutex;

    rtmp_cycle_t   *cycle;

}rtmp_event_ctx_t;


rtmp_module_t rtmp_event_module = {
    0,
    "rtmp_event_module",
    0,
    rtmp_event_create_module,
    rtmp_event_init_cycle,
    rtmp_event_init_process,
    rtmp_event_eixt_cycle
};

static void *rtmp_event_create_module(rtmp_cycle_t *cycle)
{
    rtmp_event_ctx_t  *ctx;

    ctx = mem_pcalloc(cycle->pool,sizeof(rtmp_event_ctx_t));

    if (ctx == NULL) {
        rtmp_log(RTMP_LOG_ERR,"create ctx failed");
        return NULL;
    }

    return ctx;
}

static int32_t rtmp_event_init_cycle(rtmp_cycle_t *cycle,
    rtmp_module_t *module)
{
    rtmp_event_ctx_t *ctx;
    rtmp_conf_t      *conf,*sconf;
    char            **word;
    uint32_t          conn;

    ctx = module->ctx;
    if (ctx == NULL) {
        return RTMP_FAILED;
    }

    /*init default value*/
    ctx->max_conn = 2048;
    ctx->use = EVENT_IO_SELECT;
    ctx->event_io = event_io_select;

    ctx->accept_mutex_held = 0;
    ctx->use_accept_mutex = 0;
    ctx->accept_mutex = 0;

    ctx->cycle = cycle;

    /*init conf*/
    conf = rtmp_get_conf(cycle->conf,"event",GET_CONF_CURRENT);
    if (conf == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"no event block!");
        return RTMP_OK;
    }

    sconf = rtmp_get_conf(conf,"use",GET_CONF_CHILD);
    if (sconf && sconf->argv.nelts > 1) {
        word = sconf->argv.elts;

#ifdef HAVE_EVENT_EPOLL
        if (strcmp(word[1],"epoll") == 0) {
            ctx->use = EVENT_IO_EPOLL;
            ctx->event_io = event_io_epoll;
        }
#endif

    }

    sconf = rtmp_get_conf(conf,"work_connections",GET_CONF_CHILD);
    if (sconf && sconf->argv.nelts > 1) {
        word = sconf->argv.elts;

        conn = atoi(word[1]);
        if (conn > 0) {
            ctx->max_conn = (uint32_t)conn;
        }
    }

    cycle->max_conn = ctx->max_conn;

    sconf = rtmp_get_conf(conf,"accept_mutex",GET_CONF_CHILD);
    if (sconf && sconf->argv.nelts > 1) {
        word = sconf->argv.elts;

        if ((word[1][0] == 'o' || word[1][0] == 'O')
          &&(word[1][1] == 'n' || word[1][1] == 'N')) 
        {
            ctx->use_accept_mutex = 1;
        }
    }

    return RTMP_OK;
}

static int32_t rtmp_event_init_process(rtmp_cycle_t *cycle,
    rtmp_module_t *module)
{
    rtmp_event_io_t         *io;
    rtmp_event_ctx_t        *ctx;
    rtmp_listening_t        *ls;
    int32_t                  i;
    rtmp_connection_t       *c;
    rtmp_event_t            *rev;
    
    ctx = module->ctx;

    /*create timer tree*/
    rbt_init(&ctx->timers,rtmp_timer_compare);

    io = &ctx->event_io;
    io->max_conn = ctx->max_conn;

    queue_init(&io->accept);
    queue_init(&io->posted);

    if (io->io_init) {
        if (io->io_init(io) != RTMP_OK) {
            return RTMP_FAILED;
        }
    }

    /*add accept event*/
    ls = cycle->listening.elts;
    for (i = 0;i < (int32_t)cycle->listening.nelts;i++) {
        c = ls[i].connection;

        c->listening = &ls[i];
        rev = c->read;

        rev->handler = rtmp_event_accept;
        rev->accept = 1;

        if (ctx->use_accept_mutex) {
            continue;
        }

        if (rtmp_event_add_conn(c) != RTMP_OK) {
            return RTMP_FAILED;
        }
    }

    return RTMP_OK;
}

static int32_t rtmp_event_eixt_cycle(rtmp_cycle_t *cycle,
    rtmp_module_t *module)
{
    return RTMP_OK;
}

uint32_t rtmp_event_add(rtmp_event_t *ev,int flag)
{
    rtmp_event_ctx_t    *ctx;
    rtmp_event_io_t     *io;

    ctx = rtmp_event_module.ctx;

    rtmp_log(RTMP_LOG_DEBUG,"add event:%p,%d",ev,flag);

    io = &ctx->event_io;
    if (io->io_add) {
        return io->io_add(io,ev,flag);
    }

    return RTMP_FAILED;
}

uint32_t rtmp_event_delete(rtmp_event_t *ev,int flag)
{
    rtmp_event_ctx_t    *ctx;
    rtmp_event_io_t     *io;

    ctx = rtmp_event_module.ctx;

    rtmp_log(RTMP_LOG_DEBUG,"delete event:%p,%d",ev,flag);

    io = &ctx->event_io;
    if (io->io_del) {
        return io->io_del(io,ev,flag);
    }

    return RTMP_FAILED;
}

void event_handle_event(queue_t *q)
{
    link_t          *it;
    rtmp_event_t    *ev;

    it = q->next;
    while (it != q) {
        ev = struct_entry(it,rtmp_event_t,link);

        ev->handler(ev);

        it = it->next;
    }

    queue_init(q);
}

void rtmp_event_poll(uint32_t msec)
{
    rtmp_event_ctx_t    *ctx;
    rtmp_event_io_t     *io;
    rtmp_cycle_t        *cycle;
    uint32_t             flag;
    uint64_t             delta;
    
    ctx = rtmp_event_module.ctx;
    cycle = ctx->cycle;

    io = &ctx->event_io;
    if (io->io_poll) {

        flag = 0;

        if (ctx->use_accept_mutex) {
            if (spin_trylock(&ctx->accept_mutex)) {

                if (event_io_accept_enable(cycle) == RTMP_OK) {
                    flag |= EVENT_UPDATE;
                    ctx->accept_mutex_held = 1;

                } else {
                    spin_unlock(&ctx->accept_mutex);
                    ctx->accept_mutex_held = 0;

                }
            } else {
                if (ctx->accept_mutex_held) {
                    if (event_io_accept_disable(cycle) != RTMP_OK) {
                        return ;
                    }
                }
            }
        }

        delta = rtmp_current_msec;
        io->io_poll(io,msec,flag | EVENT_UPDATE);
        delta = rtmp_current_msec - delta;

        /*handle accept*/
        event_handle_event(&io->accept);

        if (ctx->accept_mutex_held) {
            spin_unlock(&ctx->use_accept_mutex);
        }

        if (delta) {
            rtmp_event_timer_expire(& ctx->timers);
        }

        event_handle_event(&io->posted);
    }

    return ;
}

uint32_t rtmp_event_add_timer(rtmp_event_t *ev,uint32_t msec)
{
    rtmp_event_ctx_t *ctx;

    ctx = rtmp_event_module.ctx;

    ev->timeout = 0;

    if (ev->timer_set == 1) {

        rbt_remove(&ctx->timers,&ev->timer);
        ev->timer_set = 0;
    }

    
    ev->timer.k = rtmp_current_msec + msec;

    rtmp_log(RTMP_LOG_DEBUG,"add timer:%p value:%llu",ev,ev->timer.k);

    if (rbt_insert(&ctx->timers,&ev->timer,1) != 0) {
        return RTMP_FAILED;
    }
    ev->timer_set = 1;

    return RTMP_OK;
}


uint32_t rtmp_event_del_timer(rtmp_event_t *ev)
{
    rtmp_event_ctx_t *ctx;

    ctx = rtmp_event_module.ctx;

    if (!ev->timer_set) {
        return RTMP_FAILED;
    }

    rtmp_log(RTMP_LOG_DEBUG,"delete timer:%p value:%llu",ev,ev->timer.k);

    rbt_remove(&ctx->timers,&ev->timer);

    ev->timer_set = 0;

    return RTMP_OK;
}


int32_t event_io_accept_disable(rtmp_cycle_t *cycle)
{
    uint32_t           i;
    rtmp_listening_t   *ls;
    rtmp_connection_t  *c;

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {

        c = ls[i].connection;

        if (!c->read->active) {
            continue;
        }

        if (rtmp_event_delete(c->read,EVENT_READ) != RTMP_OK) {
            return RTMP_FAILED;
        }
    }
    return RTMP_OK;
}

int32_t event_io_accept_enable(rtmp_cycle_t *cycle)
{
    uint32_t           i;
    rtmp_listening_t   *ls;
    rtmp_connection_t  *c;

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {

        c = ls[i].connection;

        if (c->read->active) {
            continue;
        }

        if (rtmp_event_add(c->read,EVENT_READ) != RTMP_OK) {
            return RTMP_FAILED;
        }
    }
    return RTMP_OK;
}

void rtmp_event_accept(rtmp_event_t *ev)
{
    socket_t            fd;
    rtmp_connection_t  *c,*client;
    struct sockaddr     addr;
    struct sockaddr_in *addr_in;
    socklen_t           socklen;
    int32_t             err;
    rtmp_listening_t   *ls;
    rtmp_cycle_t       *cycle;
    rtmp_event_ctx_t   *ctx;

    c = ev->data;
    ls = c->listening;
    cycle = ls->cycle;
    ctx = rtmp_event_module.ctx;

    do {
        socklen = sizeof(addr);

        fd = accept(c->fd,&addr,&socklen);

        if (fd == -1) {
            err = sock_errno;

            if (err == ECONNABORTED) {
                rtmp_log(RTMP_LOG_INFO,"client aborted before accept()!");
                continue;
            }

            if (err == SOCK_EAGAIN) {
                rtmp_log(RTMP_LOG_DEBUG,"accept() not ready");
            }

            if (err == SOCK_EMFILE || err == SOCK_ENFILE) {
                if (event_io_accept_disable(cycle) != RTMP_OK) {
                    rtmp_log(RTMP_LOG_ERR,"disable error");
                }
            }

            return ;
        }

        if (set_nonblocking(fd) != 0) {
            rtmp_log(RTMP_LOG_WARNING,"set nonblocking failed!");
            closesocket(fd);
            continue;
        }

        if (set_tcppush(fd) != 0) {
            rtmp_log(RTMP_LOG_WARNING,"set_notcppush failed!");
            closesocket(fd);
            continue;
        }

        client = get_connection(ls,fd);
        if (client == NULL) {
            rtmp_log(RTMP_LOG_WARNING,"get_connection() failed!");
            closesocket(fd);
            continue;
        }

        client->sockaddr = addr;
        client->socklen = socklen;
        client->listening = ls;

        switch(addr.sa_family) {
        case AF_INET:
            addr_in = (struct sockaddr_in *)&addr;
            sprintf(client->addr_text,"%s",inet_ntoa(addr_in->sin_addr));
            break;
        case AF_UNIX:
            break;
        }

        ls->handler(client);
    } while (1);

    return ;
}

uint32_t rtmp_event_add_conn(rtmp_connection_t *c)
{
    rtmp_event_ctx_t    *ctx;
    rtmp_event_io_t     *io;

    ctx = rtmp_event_module.ctx;

    rtmp_log(RTMP_LOG_DEBUG,"[%d]add connection:%d",c->fd);

    io = &ctx->event_io;
    if (io->io_add_conn) {
        return io->io_add_conn(io,c);
    }

    return RTMP_FAILED;
}

uint32_t rtmp_event_delete_conn(rtmp_connection_t *c)
{
    rtmp_event_ctx_t    *ctx;
    rtmp_event_io_t     *io;

    ctx = rtmp_event_module.ctx;
    rtmp_log(RTMP_LOG_DEBUG,"[%d]delete connection:%p",c->fd,c);

    io = &ctx->event_io;
    if (io->io_del_conn) {
        return io->io_del_conn(io,c);
    }

    return RTMP_FAILED;
}