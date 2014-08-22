
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __RTMP_CORE_H_INCLUDED__
#define __RTMP_CORE_H_INCLUDED__

#include "rtmp_def.h"
#include "rtmp_array.h"
#include "rtmp_amf.h"
#include "rtmp_link.h"
#include "rtmp_bytes.h"
#include "rtmp_rbtree.h"

#include "rtmp_atomic.h"
#include "rtmp_log.h"
#include "rtmp_time.h"
#include "rtmp_error.h"

#include "rtmp_conf.h"
#include "rtmp_event.h"
#include "rtmp_event_timer.h"
#include "rtmp_core_conf.h"
#include "rtmp_chunk.h"
#include "rtmp_message.h"
#include "rtmp_host.h"
#include "rtmp_app.h"
#include "rtmp_protocol.h"
#include "rtmp_handshake.h"
#include "rtmp_session.h"
#include "rtmp_connection.h"
#include "rtmp_handler.h"

struct rtmp_cycle_s {
    mem_pool_t        *pool;
    mem_pool_t        *temp_pool;
    rtmp_conf_t       *conf;
    char              *conf_file;
        
    uint32_t           daemon;
    uint32_t           workers;
    uint32_t           max_conn;
    uint32_t           out_queue;

    rtmp_connection_t *free_connections;
    rtmp_connection_t *connections;

    rtmp_event_t      *read_events;
    rtmp_event_t      *write_events;

    array_t            ports;       /*rtmp_addr_port_t*/
    array_t            listening;   /*rtmp_listening_t*/
    array_t            server_list; /*rtmp_host_t*/
    array_t            msg_handler; /*rtmp_msg_handler_pt*/
};

struct rtmp_addr_port_s {
    uint16_t    family;
    uint16_t    port;
    array_t     addr_in;        /*rtmp_addr_inet_t*/
};

struct rtmp_addr_inet_s {
    struct sockaddr_in addr;
    array_t            hosts;   /* rtmp_host_t ** */
    rtmp_addr_port_t  *port;
};

struct rtmp_module_s{
    int32_t  index;

    char    *name;

    void    *ctx;

    void*    (*create_module)(rtmp_cycle_t *cycle);
    int32_t  (*init_cycle)(rtmp_cycle_t *cycle,rtmp_module_t *m);
    int32_t  (*init_forking)(rtmp_cycle_t *cycle,rtmp_module_t *m);
    int32_t  (*eixt_cycle)(rtmp_cycle_t *cycle,rtmp_module_t *m);
};

rtmp_cycle_t* rtmp_init_cycle();
void rtmp_run_cycle(rtmp_cycle_t *);

int32_t rtmp_server_handshake(rtmp_session_t *session);
int32_t rtmp_client_handshake(rtmp_session_t *session);

int32_t rtmp_server_handshake_done(rtmp_session_t *session);
int32_t rtmp_client_handshake_done(rtmp_session_t *session);

mem_buf_chain_t* rtmp_core_alloc_chain(rtmp_session_t *session, 
    mem_pool_t *pool,int32_t chunk_size);

void rtmp_core_lock_chain(mem_buf_chain_t *chain);

void rtmp_core_free_chain(rtmp_session_t *session,
    mem_pool_t *pool,mem_buf_chain_t *chain);

void rtmp_core_free_chains(rtmp_session_t *session,
    mem_pool_t *pool,mem_buf_chain_t *chain);

int32_t rtmp_recv_buf(int sockfd,mem_buf_t *buf,int32_t *n);
int32_t rtmp_send_buf(int sockfd,mem_buf_t *buf,int32_t *n);

void rtmp_chain_send(rtmp_event_t *ev);

int32_t rtmp_handler_init(rtmp_cycle_t *cycle);

mem_buf_t* rtmp_prepare_amf_buffer(mem_pool_t *temp_pool,
    amf_data_t **amf,uint32_t num);

uint32_t rtmp_hash_key(const u_char *data, size_t len);
uint32_t rtmp_hash_string(const char *data);

extern rtmp_module_t rtmp_core_moudle;
extern rtmp_module_t rtmp_host_moudle;
extern rtmp_module_t rtmp_event_module;

#endif
