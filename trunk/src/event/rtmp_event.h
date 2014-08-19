
/*
 * CopyLeft (C) nie950@gmail.com
 */


#ifndef __EVENT_H_INCLUDED__
#define __EVENT_H_INCLUDED__

#define EVENT_IO_SELECT            (0)  /*"select"*/
#define EVENT_IO_EPOLL             (1)  /*"epoll"*/
#define EVENT_IO_IOCP              (2)  /*"iocp"*/
#define EVENT_IO_KQUEUE            (3)  /*"kqueue"*/
#define EVENT_IO_RSING             (4)  /*"rsing"*/

#define EVENT_READ              (0x0001)
#define EVENT_WRITE             (0x0002)
#define EVENT_DONE              (0xFFFF)

#define EVENT_UPDATE            0x01
#define EVENT_POST              0x02

typedef struct rtmp_event_s rtmp_event_t;
typedef struct rtmp_event_io_s rtmp_event_io_t;

typedef void (*event_handler)(rtmp_event_t *ev);
typedef void (*connection_handler_pt)(rtmp_connection_t *c);

typedef int32_t (*event_io_init)(rtmp_event_io_t *io);
typedef int32_t (*event_io_add)(rtmp_event_io_t *io,rtmp_event_t *ev,uint32_t flag);
typedef int32_t (*event_io_add_conn)(rtmp_event_io_t *io,rtmp_connection_t *c);
typedef int32_t (*event_io_poll)(rtmp_event_io_t *io,uint32_t msec,uint32_t flag);
typedef int32_t (*event_io_del)(rtmp_event_io_t *io,rtmp_event_t *ev,uint32_t flag);
typedef int32_t (*event_io_del_conn)(rtmp_event_io_t *io,rtmp_connection_t *c);
typedef int32_t (*event_io_done)(rtmp_event_io_t *io);

struct rtmp_event_s {
    void                   *data;

    unsigned                timeout:1;
    unsigned                accept:1;
    unsigned                destroy:1;
    unsigned                timer_set:1;
    unsigned                write:1;
    unsigned                active:1;
    unsigned                ready:1;
    int16_t                 index;

    event_handler           handler;

    rbnode_t                timer;
    link_t                  link;
};

struct rtmp_event_io_s{
    uint32_t            type;
    char                name[16];

    void               *data;

    uint32_t            max_conn;

    event_io_init       io_init;
    event_io_add        io_add;
    event_io_add_conn   io_add_conn;
    event_io_poll       io_poll;
    event_io_del        io_del;
    event_io_del_conn   io_del_conn;
    event_io_done       io_done;

    /*event queue*/
    queue_t             accept;
    queue_t             posted;
};

void rtmp_event_poll(uint32_t msec);

uint32_t rtmp_event_add(rtmp_event_t *ev,int flag);
uint32_t rtmp_event_add_conn(rtmp_connection_t *c);
uint32_t rtmp_event_delete(rtmp_event_t *ev,int flag);
uint32_t rtmp_event_delete_conn(rtmp_connection_t *c);
uint32_t rtmp_event_add_timer(rtmp_event_t *ev,uint32_t msec);
uint32_t rtmp_event_del_timer(rtmp_event_t *ev);

#endif