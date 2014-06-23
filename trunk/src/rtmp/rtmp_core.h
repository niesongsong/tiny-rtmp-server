
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __RTMP_CORE_H_INCLUDED__
#define __RTMP_CORE_H_INCLUDED__


/*

C -----> C0  C1 ----> S
| <----- S0  S1 <---- |
| <-----   S2   <-----|
| ----->   C2   ----> | 

*/

#define RTMP_HANDSHAKE_FAILED         0  /*handshake failed*/

#define RTMP_HANDSHAKE_SERVER_INIT    1  /*prepare*/
#define RTMP_HANDSHAKE_SERVER_C0C1    2   /*recv c0c1*/
#define RTMP_HANDSHAKE_SERVER_S0S1    3   /*send s0s1*/
#define RTMP_HANDSHAKE_SERVER_S2      4   /*send s2*/
#define RTMP_HANDSHAKE_SERVER_C2      5   /*recv c2*/
#define RTMP_HANDSHAKE_SERVER_DONE    6

#define RTMP_HANDSHAKE_CLIENT_INIT    7  /*prepare c0c1*/
#define RTMP_HANDSHAKE_CLIENT_C0C1    8  /*send c0c1*/
#define RTMP_HANDSHAKE_CLIENT_S0S1    9  /*recv s0s1*/
#define RTMP_HANDSHAKE_CLIENT_S2     10  /*recv s2*/
#define RTMP_HANDSHAKE_CLIENT_C2     11  /*send c2*/
#define RTMP_HANDSHAKE_CLIENT_DONE   12

typedef struct rtmp_cycle_s         rtmp_cycle_t;
typedef struct rtmp_module_s        rtmp_module_t;
typedef struct rtmp_listening_s     rtmp_listening_t;
typedef struct rtmp_connection_s    rtmp_connection_t;

typedef struct rtmp_session_s       rtmp_session_t;

typedef struct rtmp_addr_port_s     rtmp_addr_port_t;
typedef struct rtmp_addr_inet_s     rtmp_addr_inet_t;

#define rtmp_abs(value)       (((value) >= 0) ? (value) : - (value))
#define rtmp_max(val1, val2)  (((val1) < (val2)) ? (val2) : (val1))
#define rtmp_min(val1, val2)  (((val1) > (val2)) ? (val2) : (val1))

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
#include "rtmp_host.h"
#include "rtmp_app.h"
#include "rtmp_handshake.h"
#include "rtmp_session.h"
#include "rtmp_connection.h"

struct rtmp_cycle_s {
    mem_pool_t        *pool;

    rtmp_conf_t       *conf;
    char              *conf_file;
        
    uint32_t           daemon;
    uint32_t           workers;
    uint32_t           max_conn;

    rtmp_connection_t *free_connections;
    rtmp_event_t      *read_event;
    rtmp_event_t      *write_event;

    array_t            ports;       /*rtmp_addr_port_t*/
    array_t            listening;   /*rtmp_listening_t*/
};

struct rtmp_addr_port_s {
    uint16_t    family;
    uint16_t    port;
    array_t     addr_in;        /*rtmp_addr_inet_t*/
};

struct rtmp_addr_inet_s {
    struct sockaddr_in addr;
    array_t            hosts;   /* rtmp_host_t ** */
};

struct rtmp_module_s{
    int32_t  index;

    char    *name;

    void    *ctx;

    void*    (*create_module)(rtmp_cycle_t *cycle);
    int32_t  (*init_cycle)(rtmp_cycle_t *cycle);
    int32_t  (*init_forking)(rtmp_cycle_t *cycle);
    int32_t  (*eixt_cycle)(rtmp_cycle_t *cycle);
};

rtmp_cycle_t* rtmp_init_cycle();
void rtmp_run_cycle(rtmp_cycle_t *);

int32_t rtmp_server_handshake(rtmp_session_t *session);
int32_t rtmp_client_handshake(rtmp_session_t *session);

int32_t rtmp_server_handshake_done(rtmp_session_t *session);
int32_t rtmp_client_handshake_done(rtmp_session_t *session);

extern rtmp_module_t rtmp_core_moudle;
extern rtmp_module_t rtmp_host_moudle;
extern rtmp_module_t rtmp_event_module;

#endif
