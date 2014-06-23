
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"


static void *rtmp_core_create_module(rtmp_cycle_t *cycle);
static int32_t rtmp_core_init_cycle(rtmp_cycle_t *cycle);
static int32_t rtmp_core_init_process(rtmp_cycle_t *cycle);
static int32_t rtmp_core_eixt_cycle(rtmp_cycle_t *cycle);

static int32_t rtmp_core_merge_listennings(rtmp_cycle_t *cycle);
static int32_t rtmp_core_open_listennings(rtmp_cycle_t *cycle);

extern void rtmp_core_init_connnect(rtmp_connection_t *conn);

#ifdef HAVE_DEBUG
void rtmp_core_dump_listennings(rtmp_cycle_t *cycle);
#endif

extern int rtmp_daemon_mode;

rtmp_module_t rtmp_core_moudle = {
    0,
    "rtmp_core_module",
    0,
    rtmp_core_create_module,
    rtmp_core_init_cycle,
    rtmp_core_init_process,
    rtmp_core_eixt_cycle
};

static void *rtmp_core_create_module(rtmp_cycle_t *cycle)
{
    cycle->daemon = CONF_OFF;
    cycle->workers = 1;
    cycle->max_conn = 2048;

    array_init(& cycle->ports,cycle->pool,10,sizeof(rtmp_addr_port_t));
    array_init(& cycle->listening,cycle->pool,10,sizeof(rtmp_listening_t));

    return (void *)-1;
}

static int32_t rtmp_core_init_cycle(rtmp_cycle_t *cycle)
{
    rtmp_conf_t      *sconf;
    char            **word;
    
    sconf = rtmp_get_conf(cycle->conf,"daemon",GET_CONF_CURRENT);
    if (sconf == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"no daemon!");
    }

    if (sconf && sconf->argv.nelts > 1) {
        word = sconf->argv.elts;

#ifndef HAVE_OS_WIN32
        if ((word[1][0] == 'o' || word[1][1] == 'O')
          &&(word[1][0] == 'n' || word[1][1] == 'N')) 
        {
            cycle->daemon = CONF_ON;
        }
#endif

    }

    sconf = rtmp_get_conf(cycle->conf,"workers",GET_CONF_CURRENT);
    if (sconf == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"no daemon!");
    }

    if (sconf && sconf->argv.nelts > 1) {
        word = sconf->argv.elts;

#ifndef HAVE_OS_WIN32
        if (strcmp(word[1],"workers") == 0) {
            uint32_t          n;

            n = atoi(word[1]);
            if (n > 0) {
                cycle->workers = (uint32_t)n;
            }
        }
#endif

    }

    return RTMP_OK;
}

static int32_t rtmp_core_init_process(rtmp_cycle_t *cycle)
{
    mem_pool_t         *pool;
    int32_t             conn,i;
    rtmp_connection_t  *next;

    pool = cycle->pool;
    conn = (int32_t)cycle->max_conn;

    cycle->read_event = mem_pcalloc(pool, sizeof(rtmp_event_t)*conn);
    if (cycle->read_event == NULL) {
        return RTMP_FAILED;
    }

    cycle->write_event = mem_pcalloc(pool, sizeof(rtmp_event_t)*conn);
    if (cycle->write_event == NULL) {
        return RTMP_FAILED;
    }

    cycle->free_connections = mem_pcalloc(pool,
        sizeof(rtmp_connection_t) * conn);
    if (cycle->free_connections == NULL) {
        return RTMP_FAILED;
    }

    next = NULL;
    for (i = conn-1;i >= 0 ;i--) {
        cycle->write_event[i].write = 1;
        cycle->read_event[i].write = 0;

        cycle->free_connections[i].next = next;
        next = & cycle->free_connections[i];

        next->read  = & cycle->read_event[i];
        next->write = & cycle->write_event[i];
    }

#if HAVE_DEBUG
    rtmp_core_dump_listennings(cycle);
#endif

    /*merge listening*/
    if (rtmp_core_merge_listennings(cycle) != RTMP_OK) {
        return RTMP_FAILED;
    }

    /*open listening*/
    if (rtmp_core_open_listennings(cycle) != RTMP_OK) {
        return RTMP_FAILED;
    }

    return RTMP_OK;
}

static int32_t rtmp_core_eixt_cycle(rtmp_cycle_t *cycle)
{
    return RTMP_OK;
}

static int32_t rtmp_core_add_listening(rtmp_cycle_t *cycle,
    rtmp_addr_inet_t *addr)
{
    rtmp_listening_t    *ls;

    ls = array_push(&cycle->listening);
    memset(ls,0,sizeof(rtmp_listening_t));

    ls->cycle = cycle;

    ls->fd = rtmp_socket(AF_INET,SOCK_STREAM,0);
    if (ls->fd == -1) {
        rtmp_log(RTMP_LOG_WARNING,"socket() failed");
        return RTMP_FAILED;
    }

    ls->handler = rtmp_core_init_connnect;

    ls->data = addr;

    ls->sockaddr = *(struct sockaddr*)(& addr->addr);
    ls->socklen = sizeof(ls->sockaddr);
    
    sprintf(ls->sockaddr_text,"%s",
        inet_ntoa(addr[0].addr.sin_addr));

    return RTMP_OK;
}

static int32_t rtmp_core_merge_listennings(rtmp_cycle_t *cycle)
{
    rtmp_addr_port_t    *ports;
    rtmp_addr_inet_t    *addr;
    int32_t              i,j,h;
    rtmp_host_t        **host;
    void               **n;

    ports = cycle->ports.elts;
    for (i = 0 ;i < (int32_t)cycle->ports.nelts;i++) {
        if (ports[i].family != AF_INET) {
            continue;
        }

        addr = ports[i].addr_in.elts;
        if (addr[0].addr.sin_addr.s_addr == INADDR_ANY) {
            rtmp_core_add_listening(cycle,addr);

            for (j = 1;j < (int32_t)ports[i].addr_in.nelts; j++) {

                n = array_push_n(&addr[0].hosts,addr[j].hosts.nelts);

                host = addr[j].hosts.elts;
                for (h = 0; h < (int32_t)addr[j].hosts.nelts;h++) {
                    *n++ = host[h];
                }
            }

            continue;
        }

        for (j = 0;j < (int32_t)ports[i].addr_in.nelts; j++) {
            rtmp_core_add_listening(cycle,& addr[j]);
        }
    }

    return RTMP_OK;
}

static int32_t rtmp_core_open_listennings(rtmp_cycle_t *cycle)
{
    int32_t              i;
    rtmp_listening_t    *ls;
    rtmp_connection_t   *conn;

    ls = cycle->listening.elts;

    for (i = 0;i < (int32_t)cycle->listening.nelts;i++) {
        if (ls[i].fd == -1) {
            continue;
        }

        conn = get_connection(ls,ls[i].fd);
        if (conn == NULL) {
            rtmp_log(RTMP_LOG_ERR,"get_connection() failed");
            continue;
        }

        ls[i].connection = conn;
        
        /*set nonblock*/
        if (set_nonblocking(ls[i].fd) != 0) {
            rtmp_log(RTMP_LOG_ERR,"set_nonblocking() failed!%d",sock_errno);
            return RTMP_FAILED;
        }

        if (bind(ls[i].fd,& ls[i].sockaddr,ls[i].socklen) == -1) {
            rtmp_log(RTMP_LOG_ERR,"bind() failed!%d",sock_errno);
            return RTMP_FAILED;
        }

        if (listen(ls[i].fd,cycle->max_conn) == -1) {
            rtmp_log(RTMP_LOG_ERR,"listen() failed!%d",sock_errno);
            return RTMP_FAILED;
        }
    }

    return RTMP_OK;
}

#ifdef HAVE_DEBUG

void rtmp_core_dump_listennings(rtmp_cycle_t *cycle)
{  
    rtmp_addr_port_t *ports;
    rtmp_addr_inet_t *addr;
    rtmp_host_t     **host;
    int32_t           i,j,h;

    ports = cycle->ports.elts;
    for (i = 0 ;i < (int32_t)cycle->ports.nelts;i++) {
        printf("port: %d\n",ntohs(ports[i].port));

        addr = ports[i].addr_in.elts;
        for (j = 0;j < (int32_t)ports[i].addr_in.nelts; j++) {
            printf("%s\n",inet_ntoa(addr[j].addr.sin_addr));

            host = addr[j].hosts.elts;
            for (h = 0; h < (int32_t)addr[j].hosts.nelts;h++) {
                printf("\t%s\n",host[h]->name);
            }
        }
    }
}

#endif