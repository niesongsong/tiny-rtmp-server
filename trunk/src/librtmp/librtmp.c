
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "librtmp_in.h"

#define RTMP_ROLE_NULL      0
#define RTMP_ROLE_PULL      1
#define RTMP_ROLE_PUSH      2

#define RTMP_MAX_STREAMS    (1024)

typedef struct rtmp_connection_s rtmp_connection_in_t;
typedef struct rtmp_session_connect_s rtmp_session_connect_t;
typedef struct rtmp_session_stream_s rtmp_session_stream_t;
typedef struct rtmp_sock_opt_s rtmp_sock_opt_t;

typedef struct rtmp_header_s {
    uint16_t    format;             /*00 01 10 11*/
    uint16_t    type_id;            
    uint32_t    chunk_stream_id;
    uint32_t    timestamp;
    uint32_t    body_size;
    uint32_t    stream_id;
} rtmp_header_t;

struct rtmp_session_stream_s {
    int                   stream_id;
    char                 *name;
    rtmp_connection_in_t *conn;

    /*last message information*/
    uint32_t              last_stream_time;
    uint32_t              last_msg_id;
    uint16_t              last_msg_len;
};

struct rtmp_session_connect_s {
    char    *app;
    char    *args;
    char    *flashver;
    char    *swf_url;
    char    *tc_url;
    char    *page_url;
    char    *vhost;

    double   trans;
    double   acodecs;
    double   vcodecs;
    double   object_encoding; 
};

struct rtmp_sock_opt_s {
    struct sockaddr sa;

    uint16_t        nonblocking : 1;    /*none blocking*/
    uint16_t        quit : 1;           /*quit flag*/

    int16_t         port;
    socket_t        fd;
    
    char           *addr_text;
    char           *host;

    uint32_t        msec;               /*poll timeout*/
};

struct rtmp_connection_s {
    int                     role;
    
    rtmp_session_connect_t  conn;
    rtmp_sock_opt_t         sock_opt;

    mem_pool_t             *pool;
    mem_pool_t             *temp_pool;

    rtmp_session_stream_t   sys_stream; /*stream id: 0*/
    int                     max_streams;
    list_t                 *streams;

    mem_buf_t               handshake[6];

    int                     lock;
};

static int rtmp_set_nonblocking(rtmp_sock_opt_t *opt);
static int rtmp_connect_host(rtmp_sock_opt_t *opt);

static int rtmp_send_buf(rtmp_sock_opt_t *opt, mem_buf_t *buf);
static int rtmp_recv_buf(rtmp_sock_opt_t *opt, mem_buf_t *buf);

static int rtmp_send_n(rtmp_sock_opt_t *opt, const char *buf,int n);
static int rtmp_recv_n(rtmp_sock_opt_t *opt, char *buf,int n);

static int rtmp_create_sock(rtmp_sock_opt_t *opt);
static void rtmp_delete_sock(rtmp_sock_opt_t *opt);
static int rtmp_parse_app(rtmp_connection_in_t *conn,const char *app);
static int rtmp_handshake(rtmp_connection_in_t *conn);
static int rtmp_handshake_v2(rtmp_connection_in_t *conn);
static mem_buf_t* rtmp_create_connect_amf(rtmp_connection_in_t *conn);

static void *rtmp_temp_pmalloc(size_t size,void *u);
static void rtmp_temp_pfree(void *p,void *u);

rtmp_net_conn_t rtmp_conn_create(
    rtmp_malloc_rt a,rtmp_free_rt f,void *u)
{
    mem_pool_t        *pool,*temp_pool;
    rtmp_connection_in_t *conn;
    int                i;

    pool = mem_create_pool(0);
    temp_pool = mem_create_pool(0);

    if (pool == NULL || temp_pool == NULL) {
        goto alloc_failed;
    }

    conn = (rtmp_connection_in_t *)mem_pcalloc(pool,
        sizeof(rtmp_connection_in_t));

    if (conn == NULL) {

        rtmp_log(RTMP_LOG_ERR,"mem_pcalloc(%d) failed",
            sizeof(rtmp_connection_in_t));

        goto alloc_failed;
    }

    conn->role = RTMP_ROLE_NULL;
    conn->pool = pool;
    conn->temp_pool = temp_pool;
    conn->max_streams = RTMP_MAX_STREAMS;

    conn->streams = (list_t*)mem_pcalloc(pool,sizeof(list_t) *
        conn->max_streams);

    /*create system stream*/
    conn->sys_stream.stream_id = 0;
    conn->sys_stream.name = "system control";
    conn->sys_stream.conn = conn;

    if (conn->streams == NULL) {
        goto alloc_failed;
    }

    for (i = 0;i < conn->max_streams;i++) {
        list_init(conn->streams+i);
    }
    
    return conn;

alloc_failed:

    if (pool) {
        mem_destroy_pool(pool);
    }

    if (temp_pool) {
        mem_destroy_pool(temp_pool);
    }

    return NULL;
}

int rtmp_conn_connect(rtmp_net_conn_t c,const char *app)
{
    int                   rc;
    rtmp_connection_in_t *conn;
    mem_buf_t            *buf;
    rtmp_message_t         msg;

    /*connect() | windows_ack_size()*/
    if (c == NULL) {
        return RTMP_ERR_ERROR;
    }

    conn = (rtmp_connection_in_t*)c;
    rc = rtmp_parse_app(conn,app);
    if (rc != RTMP_ERR_OK) {
        return RTMP_ERR_ERROR;
    }

    rc = rtmp_create_sock(&conn->sock_opt);
    if (rc != RTMP_ERR_OK) {
        goto alloc_failed;
    }

    rc = rtmp_handshake(conn);
    if (rc != RTMP_ERR_OK) {
        goto alloc_failed;
    }

    /*create connect app amf*/
    buf = rtmp_create_connect_amf(conn);
    if (buf == NULL) {
        goto alloc_failed;
    }

    memset(&msg,0,sizeof(msg));

    msg.body = buf->buf;
    msg.body_size = buf->last - buf->buf;
    msg.type = RTMP_MESSAGE_TYPE_AMF0;

    return rtmp_stream_send(&conn->sys_stream,&msg);

alloc_failed:

    if (conn->sock_opt.fd != -1) {
        closesocket(conn->sock_opt.fd);
    }

    return RTMP_ERR_ERROR;
}

int rtmp_conn_close(rtmp_net_conn_t conn)
{
    return 0;
}

int rtmp_conn_destroy(rtmp_net_conn_t conn)
{
    return 0;
}

/*NetStream*/
rtmp_net_stream_t rtmp_stream_create(rtmp_net_conn_t conn,
    const char *name)
{
    /*releaseStream() | FCPublish() | createStream()*/



    return NULL;
}


int rtmp_stream_play(rtmp_net_stream_t stream)
{
    return 0;
}

int rtmp_stream_publish(rtmp_net_stream_t stream)
{
    return 0;
}

int rtmp_stream_recv(rtmp_net_stream_t stream,rtmp_message_t *msg)
{
    return 0;
}

int rtmp_stream_send(rtmp_net_stream_t stream,rtmp_message_t *msg)
{
    rtmp_session_stream_t *s;

    /*stream time last stream*/
    





    return 0;
}

int rtmp_stream_close(rtmp_net_stream_t stream)
{
    return 0;
}

int rtmp_stream_destroy(rtmp_net_stream_t stream)
{
    return 0;
}

int rtmp_parse_app(rtmp_connection_in_t *conn,const char *app)
{
    
    int      len;
    uint16_t port;
    char    *tc_url,*u,*s,*colon;

    if (app == NULL) {
        return RTMP_ERR_ERROR;
    }

    len = strlen(app);
    tc_url = mem_dup_case_str(app,0,conn->pool);
    if (tc_url == NULL) {
        return RTMP_ERR_ERROR;
    }

    if (memcmp(tc_url,rtmp_proto_n,rtmp_proto_len)) {
        return RTMP_ERR_ERROR;
    }

    conn->conn.tc_url = tc_url;
    u = tc_url + rtmp_proto_len;
    s = strchr(u,'/');   /*host[:port]*/
    colon = strchr(u,':');

    if (s == NULL || u == NULL) {
        return RTMP_ERR_FAILED;
    }

    conn->sock_opt.port = 1935;
    conn->sock_opt.host = mem_dup_str_size(u,s-u,conn->pool);

    if (conn->sock_opt.host == NULL) {
        return RTMP_ERR_FAILED;
    }

    if (colon && colon < s) {

        port = atoi(colon+1);
        conn->sock_opt.host[colon-u] = '\0';

        if (port != 0) {
            conn->sock_opt.port = port;
        }
    }

    conn->conn.app = mem_dup_str(++s,conn->pool);
    conn->conn.swf_url = mem_dup_str("http://localhost/swfs/LiveSample.swf",
        conn->pool);
    conn->conn.page_url = mem_dup_str("http://localhost/",conn->pool);
    conn->conn.flashver = mem_dup_str("WIN 13,0,0,214",conn->pool);

    conn->conn.object_encoding = 3.0;
    conn->conn.acodecs = 3575.0;
    conn->conn.vcodecs = 252.0;
    conn->conn.trans = 1.0;

    return RTMP_ERR_OK;
}

int rtmp_create_sock(rtmp_sock_opt_t *opt)
{
    struct hostent     *host;
    struct sockaddr_in *sa,si;
    socklen_t           socklen;
    int                 rc, err;
    socket_t            fd;

    if (opt->host == NULL) {
        return RTMP_ERR_ERROR;
    }

    host = gethostbyname(opt->host);
    if (host == NULL) {

        rtmp_log(RTMP_LOG_ERR,"gethostbyname(%s) failed: %d",
            opt->host,sock_errno);

        return RTMP_ERR_FAILED;
    }

    sa = (struct sockaddr_in *)&opt->sa;

    sa->sin_addr = *((struct in_addr*)host->h_addr);
    sa->sin_port = htons(opt->port);
    sa->sin_family = AF_INET;

    fd = socket(AF_INET,SOCK_STREAM,0);

    if (fd == -1) {
        return RTMP_ERR_FAILED;
    }

    opt->nonblocking = 1;
    opt->quit        = 0;
    opt->msec        = 5000;
    opt->fd          = fd;
    
    rc = rtmp_set_nonblocking(opt);

    if (rc == -1) {

        rtmp_log(RTMP_LOG_ERR,"rtmp_set_nonblocking() failed: %d",
            opt->nonblocking);

        goto sock_err;
    }

    /*connect*/
    socklen = sizeof(opt->sa);
    rc = rtmp_connect_host(opt);
    if (rc != 0) {

        rtmp_log(RTMP_LOG_ERR,"rtmp_connect_host() failed: %d",
            opt->nonblocking);

        goto sock_err;
    }

    socklen = sizeof(si);
    getpeername(opt->fd,(struct sockaddr*)&si, &socklen);

    return RTMP_ERR_OK;

sock_err:

    err = sock_errno;
    closesocket(opt->fd);
    opt->fd = -1;

    return err;
}

static void rtmp_delete_sock(rtmp_sock_opt_t *opt)
{
    if (opt && opt->fd != -1) {
        closesocket(opt->fd);
        opt->fd = -1;
    }
}

/*
 +------+              +------+
 |client|              |server|
 +------+              +------+
    | -----(C0, C1)---->  |
    | <----(S0, S1)-----  |
    | <----(  S2  )-----  |
    | -----(  C2  )---->  |

    C0：(协议版本)0x03;
    C1：4B(时间1) + 4B(全零)  + 1528B(随机数1)
    S0：(协议版本)0x03;
    S1：4B(时间2) + 4B(全零)  + 1528B(随机数2)
    C2：4B(时间2) + 4B(时间4) + 1528B(随机数2)
    S2：4B(时间1) + 4B(时间3) + 1528B(随机数1)

*/
int rtmp_handshake(rtmp_connection_in_t *conn)
{
    uint8_t    *c0,*c1,*c2,*s0,*s1,*s2;
    size_t      sent,n,rcv;

    c0 = (uint8_t *)mem_pcalloc(conn->temp_pool,
        (1+1536+1536)*2);
    if (c0 == NULL) {
        return RTMP_ERR_ERROR;
    }

    c1 = c0 + 1;
    c2 = c1 + 1536;
    s0 = c2 + 1536;
    s1 = s0 + 1;
    s2 = s1 + 1536;

    c0[0] = 0x03;
    ulong_make_byte4(c1, rtmp_current_sec);
    memset(c1 + 4, 0, 4);
    byte_fill_random(c1+8,1528);

    /*send c0 c1*/
    n = 1537;
    sent = rtmp_send_n(&conn->sock_opt, (const char *)c0, n);
    if (sent != n) {
        rtmp_log(RTMP_LOG_ERR,"send() failed", sock_errno);
        return RTMP_ERR_FAILED;
    }

    /*recv s0 s1*/
    n = 1537;
    rcv = rtmp_recv_n(&conn->sock_opt,(char*)s0,n);
    if (rcv < n) {

        rtmp_log(RTMP_LOG_ERR,"recv(%d) failed: %d bytes recvd",
            conn->sock_opt.fd,rcv);

        return RTMP_ERR_FAILED;
    }
    rtmp_time_update();

    /*send c2*/
    memcpy(c2,s1,4);
    ulong_make_byte4(c1,rtmp_current_sec);
    memcpy(c2+8,s1+8,1528);

    n = 1536;
    sent = rtmp_send_n(&conn->sock_opt,(const char*)c2,n);
    if (sent < n) {

        rtmp_log(RTMP_LOG_ERR,"send(%d) failed: %d bytes sent",
            conn->sock_opt.fd,sent);

        return RTMP_ERR_FAILED;
    }

    /*recv s2: discard*/
    n = 1536;
    rcv = rtmp_recv_n(&conn->sock_opt,(char*)s2,n);
    if (rcv < n) {

        rtmp_log(RTMP_LOG_ERR,"recv(%d) failed: %d bytes recvd",
            conn->sock_opt.fd,rcv);

        return RTMP_ERR_FAILED;
    }

    return RTMP_ERR_OK;
}

mem_buf_t *rtmp_create_connect_amf(rtmp_connection_in_t *conn)
{
    amf_data_t *amf[3];
    mem_buf_t  *buf;
    char       *c;
    int32_t     n,r,i;
    
    amf_set_allocator(rtmp_temp_pmalloc,rtmp_temp_pfree,
        conn->temp_pool);

    amf[0] = amf_new_string("connect",7);
    amf[1] = amf_new_number(1.0);
    amf[2] = amf_new_object();

    amf_put_prop(amf[2],"videoFunction",amf_new_number(1.0));
    amf_put_prop(amf[2],"videoCodecs",amf_new_number(conn->conn.vcodecs));
    amf_put_prop(amf[2],"audioCodecs",amf_new_number(conn->conn.acodecs));
    amf_put_prop(amf[2],"fpad",amf_new_bool(0));
    amf_put_prop(amf[2],"capabilities",amf_new_number(239.0));

    amf_put_prop(amf[2],"flashVer",amf_new_string_c(conn->conn.flashver));
    amf_put_prop(amf[2],"tcUrl",amf_new_string_c(conn->conn.tc_url));
    amf_put_prop(amf[2],"swfUrl",amf_new_string_c(conn->conn.swf_url));
    amf_put_prop(amf[2],"pageUrl",amf_new_string_c(conn->conn.page_url));
    amf_put_prop(amf[2],"app",amf_new_string_c(conn->conn.app));  

    amf_put_prop(amf[2],"objectEncoding",
        amf_new_number(conn->conn.object_encoding));

    amf_set_allocator(NULL,NULL,NULL);
    amf_dump_data(amf[2]);

    if (!amf[0] || !amf[1] || !amf[2]) {
        return NULL;
    }

    /*serialize*/
    buf = mem_buf_palloc(conn->pool,NULL,2048);
    if (buf == NULL) {
        return NULL;
    }

    for (i = 0;i < (int)_array_length(amf);i++) {

        n = buf->end - buf->last;
        c = (char*)buf->last;

        r = amf_encode(amf[i],c,n);
        if (r <= 0) {
            return NULL;
        }

        buf->last += n - r;
    }

    return buf;
}

void *rtmp_temp_pmalloc(size_t size,void *u)
{
    mem_pool_t *pool = (mem_pool_t *)u;
    return mem_palloc(pool,size);
}

void rtmp_temp_pfree(void *p,void *u)
{
    return ;
}

/*send (buf->last ---> buf->end)*/
int rtmp_send_buf(rtmp_sock_opt_t *opt, mem_buf_t *buf)
{
    uint32_t     n, sum;
    int          r;
    int32_t      msec, interval;

    interval = 30;
    sum      = 0;
    msec     = opt->msec;

    while (opt->quit == 0 && buf->last != buf->end) {

        n = buf->end - buf->last;
        r = send(opt->fd, (const char*)buf->last, n, 0);
        
        if (r <= 0) {
            if (sock_errno == EBLOCK) {

                msleep(interval);
                msec -= interval;

                if (msec < 0) {
                    return sum;
                }

                continue;
            }

            return -1;
        }

        buf->last += r;
        sum += r;
    }

    return sum;
}

/*send (buf->last ---> buf->end)*/
int rtmp_recv_buf(rtmp_sock_opt_t *opt, mem_buf_t *buf)
{
    uint32_t     n, sum;
    int          r;

    sum = 0;
    while (opt->quit == 0 && buf->last != buf->end) {

        n = buf->end - buf->last;
        r = recv(opt->fd, (char *)buf->last, n, 0);

        if (r <= 0) {
            if (sock_errno == EBLOCK) {
                return sum;
            }

            return -1;
        }

        buf->last += r;
        sum += r;
    }

    return sum;
}

int rtmp_send_n(rtmp_sock_opt_t *opt,const char *buf,int n)
{
    mem_buf_t   b;

    b.buf = b.last = (u_char *)buf;
    b.end = (u_char *)buf + n;
 
    return rtmp_send_buf(opt, &b);
}

int rtmp_recv_n(rtmp_sock_opt_t *opt,char *buf,int n)
{
    mem_buf_t   b;
    int         rc, msec, interval;

    b.buf = b.last = (u_char *)buf;
    b.end = (u_char *)buf + n;

    interval = 30;
    msec     = opt->msec;

    while (msec > 0 && b.last != b.end) {

        rc = rtmp_recv_buf(opt, &b);

        if (rc < 0) {
            return -1;
        }

        if (rc == 0) {
            msleep(interval);
            msec -= interval;
        }
    }

    return b.last - b.buf;
}

int rtmp_set_nonblocking(rtmp_sock_opt_t *opt)
{
    socket_t    fd;
    uint16_t    nonblocking;

    fd          = opt->fd;
    nonblocking = opt->nonblocking;

#ifdef WIN32 
    {
    unsigned long nb;
    nb = nonblocking ? 1 : 0;

    return ioctlsocket(fd, FIONBIO, &nb);
    }
#else
    {
    int fl;
    fl = nonblocking ? 
        (fcntl(fd, F_GETFL) | O_NONBLOCK) : (fcntl(fd, F_GETFL) & ~O_NONBLOCK);

    return fcntl(fd, F_SETFL , fl);
    }
#endif
}

int rtmp_connect_host(rtmp_sock_opt_t *opt)
{
    socket_t        fd;
    int             rc, ready, err, msec, len;
    fd_set          wfs, efs;
    struct timeval  tv;

    fd = opt->fd;
    

    rc = connect(fd, & opt->sa, sizeof(struct sockaddr));

    if (opt->nonblocking == 0 || rc == 0) {
        return rc;
    }

    len  = sizeof(int);
    msec = opt->msec;
    while (msec > 0 && opt->quit == 0) {

        FD_ZERO(&wfs);
        FD_ZERO(&efs);

        FD_SET(fd,&wfs);
        FD_SET(fd,&efs);

        tv.tv_usec = 100000; /*100ms*/
        tv.tv_sec  = 0;

        ready = select(fd + 1, NULL, &wfs, &efs, &tv);

        if (ready == -1) {
            return -1;
        }

        if (ready == 0) {
            msec -= 100;
            continue;
        }

        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &err, 
            (socklen_t *)&len) == 0) 
        {
            /*connect success*/
            if (err == 0) {
                return 0;
            }
        }

        /*connect failed*/
        return -1;
    }

    /*connect timeout or canceled*/
    return -2;
}
