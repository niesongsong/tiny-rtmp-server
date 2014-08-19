
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#define HAVE_USING_LEDIAN

#include "librtmp.h"
#include "rtmp_link.h"
#include "rtmp_handshake.h"

#include "rtmp_amf.h"
#include "rtmp_bytes.h"

#define rtmp_min(x,y)           ((x)<(y)?(x):(y))
#define rtmp_max(x,y)           ((x)>(y)?(x):(y))

#define rtmp_proto_n            "rtmp://"
#define rtmp_proto_len          (sizeof(rtmp_proto_n)-1)

#define RTMP_URL_MAX_LEN        (1024)
#define RTMP_URL_HOST_LEN       (256)
#define RTMP_URL_APP_LEN        (256)

typedef struct rtmp_in_s rtmp_in_t;
struct rtmp_in_s {

    char                url[RTMP_URL_MAX_LEN];

    char                host[RTMP_URL_HOST_LEN];
    char                app[RTMP_URL_APP_LEN];
    char                tcUrl[RTMP_URL_MAX_LEN];

    char               *stream;
    
    struct sockaddr     addr;
    uint16_t            port;

    rtmp_malloc_rt      alloc;
    rtmp_free_rt        free;
    void               *data;

    
    char               *csbuf[2];           /*handshake buffer*/

    queue_t             recv;              /*recv queue*/
    queue_t             send;              /*send queue*/

    int                 sockfd;            /*server fd*/

    rtmp_handshake_t    handshake;         /*handshake*/
};

static int rtmp_handshake_in(rtmp_t rtmp,int type);
static int rtmp_parse_url(rtmp_in_t *r);
static int rtmp_send_n(int sockfd,const char *buf,int n);
static int rtmp_recv_n(int sockfd,char *buf,int n);

static int rtmp_send_n(int sockfd,const char *buf,int n)
{
    int sent,rc;

    sent = 0;
    while (n > 0) {
        rc = send(sockfd,buf+sent,n,0);

        if (rc <= 0) {
            break;
        }

        sent += rc;
        n -= sent;
    }

    return sent;
}

static int rtmp_recv_n(int sockfd,char *buf,int n)
{
    int recvd,rc;

    recvd = 0;
    while (n > 0) {
        rc = recv(sockfd,buf+recvd,n,0);

        if (rc <= 0) {
            break;
        }

        recvd += rc;
        n -= recvd;
    }

    return recvd;
}

static int rtmp_parse_url(rtmp_in_t *r)
{
    char *u,*s,*t,*p,*q,*colon;

    r->port = 1935;

    if (memcmp(r->url,rtmp_proto_n,rtmp_proto_len)) {
        return RTMP_PROTOCOL;
    }

    u = r->url + rtmp_proto_len;

    s = strchr(u,'//');   /*host[:port]*/
    colon = strchr(u,':');
    t = s;

    p = q = 0;

    /*last '?' or '/'*/
    while ((*t) && (*t != '?')) {
        if (*t == '/') {
            p = q;
            q = t;
        }
        t++;
    }
    if (*t == 0) {
        p = q;
        q = t;
    }

    if (!p || !q) {
        return RTMP_INVALID_URL;
    }

    /*[u,s): host:port*/
    r->port = 1935;
    memcpy(r->host,u,s-u);

    if (colon && colon < s) {
        uint16_t port;

        r->host[colon-u] = '\0';

        port = atoi(colon+1);
        if (port != 0) {
            r->port = port;
        }
    }
    s++;

    /*(s,p): app*/
    memcpy(r->app,s,p-s);

    /*[u,p): tcUrl*/
    memcpy(r->tcUrl,u,p-u);

    /*(p,q]: stream*/
    r->stream = p + 1;

    return RTMP_OK;
}

rtmp_t rtmp_create(const char *url,rtmp_malloc_rt alloc,rtmp_free_rt f,void *u)
{
    rtmp_in_t t,*r;
    int len;
    struct hostent *host;
    struct sockaddr_in *sa;

    if (!url || !alloc && !f) {
        return RTMP_NULL;
    }
    
    len = strlen(url);
    if ((len <= rtmp_proto_len) || (len > RTMP_URL_MAX_LEN)) {
        return RTMP_NULL;
    }

    memset(&t,0,sizeof(rtmp_in_t));

    t.alloc = alloc;
    t.free = f;
    t.data = u;

    memcpy(t.url, url, len);

    if (rtmp_parse_url(&t) != RTMP_OK) {
        return RTMP_NULL;
    }

    host = gethostbyname(t.host);
    if (host == NULL) {
        return RTMP_NULL;
    }

    sa = (struct sockaddr_in *)&t.addr;

    sa->sin_addr = *((struct in_addr*)host->h_addr);
    sa->sin_port = htons(t.port);
    sa->sin_family = AF_INET;

    t.sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (t.sockfd == -1) {
        return RTMP_NULL;
    }

    r = t.alloc(sizeof(rtmp_in_t)+RTMP_HS_LEN,t.data);
    if (r == NULL) {
        closesocket(t.sockfd);
        return RTMP_NULL;
    }

    memcpy(r,&t,sizeof(rtmp_in_t));

    r->csbuf[0] = (char*)r + sizeof(rtmp_in_t);
    r->csbuf[1] = (char*)r->csbuf[0] + RTMP_HS_C_LEN;

    return r;
}

void rtmp_destroy(rtmp_t rtmp)
{
    return ;
}

/*handshake with server*/
int rtmp_handshake(rtmp_t rtmp)
{
    return rtmp_handshake_in(rtmp,RTMP_HANDSHAKE_V1);
}

/*handshake with server*/
int rtmp_handshake_v2(rtmp_t rtmp)
{
    return rtmp_handshake_in(rtmp,RTMP_HANDSHAKE_V2);
}

static int rtmp_handshake_in(rtmp_t rtmp,int type)
{
    rtmp_in_t *r;
    int socklen,rc,n,recvd,sent;
    char *hc,*c2;
    rtmp_handshake_t *handshake;

    if (rtmp == RTMP_NULL) {
        return RTMP_FAILED;
    }

    r = (rtmp_in_t*)rtmp;
    handshake = & r->handshake;

    rtmp_handshake_init(handshake,2,1);

    /*connect server*/
    socklen = sizeof(r->addr);
    rc = connect(r->sockfd,&r->addr,socklen);
    if (rc == -1) {
        rc = WSAGetLastError();
        return rc;
    }

    /*create c0 + c1*/
    rtmp_create_c0(handshake,r->csbuf[0]);
    rtmp_create_c1(handshake,r->csbuf[0]);

    /*send c0 + c1*/
    sent = rtmp_send_n(r->sockfd,r->csbuf[0],RTMP_C0_LEN + RTMP_C1_LEN);
    if (sent < RTMP_C0_LEN + RTMP_C1_LEN) {
        rc = WSAGetLastError();
        return rc;
    }

    hc = r->csbuf[1];
    n = RTMP_S0_LEN + RTMP_S1_LEN + RTMP_S2_LEN;

    recvd = rtmp_recv_n(r->sockfd,hc,n);

    if (recvd < n) {
        rc = WSAGetLastError();
        return rc;
    }
    
    /*s1c1 time*/
    handshake->c1s1_time = (uint32_t)time(0);

    /*verify*/
    rc = rtmp_create_c2(handshake,r->csbuf[0],r->csbuf[1]);
    if (rc != RTMP_OK) {
        return RTMP_HANDSHAKE;  /*hand shake failed*/
    }

    c2 = r->csbuf[0]+RTMP_C0_LEN + RTMP_C1_LEN;

    /*send C2*/
    sent = rtmp_send_n(r->sockfd,c2,RTMP_C2_LEN);
    if (sent < RTMP_C2_LEN) {
        rc = WSAGetLastError();
        return rc;
    }

    return 0;   
}

/*connect and set windows acknowledgment size(2500000)*/
int rtmp_connect_app(rtmp_t rtmp)
{
    amf_data_t *conn,*id, *obj;
    rtmp_in_t  *r;
    char       *buf,*c;
    int         remain,ec;

    r = (rtmp_in_t *)rtmp;

    buf = r->alloc(2048,r->data);
    if (buf == NULL) {
        return RTMP_FAILED;
    }

    conn = amf_new_string("connect",7);
    id = amf_new_number(1.0);

    obj = amf_new_object();

    amf_put_prop(obj,"videoFunction",amf_new_number(1.0));
    amf_put_prop(obj,"videoCodecs",amf_new_number(252.0));
    amf_put_prop(obj,"tcUrl",amf_new_string(r->tcUrl,strlen(r->tcUrl)));
    amf_put_prop(obj,"swfUrl",amf_new_string("http://localhost/swfs/LiveSample.swf",36));
    amf_put_prop(obj,"pageUrl",amf_new_string("http://localhost/",17));
    amf_put_prop(obj,"objectEncoding",amf_new_number(3.0));
    amf_put_prop(obj,"fpad",amf_new_bool(0));
    amf_put_prop(obj,"flashVer",amf_new_string("WIN 13,0,0,214",14));
    amf_put_prop(obj,"capabilities",amf_new_number(239.0));
    amf_put_prop(obj,"audioCodecs",amf_new_number(3575.0));
    amf_put_prop(obj,"app",amf_new_string(r->app,strlen(r->app)));
    amf_put_prop(obj,"",amf_new_object_end());

    ec = 2048;
    c = buf;

    remain = amf_encode(conn,c,ec);
    if (remain <= 0) {
        goto invalid;
    }
    c += ec - remain;
    ec = remain;

    remain = amf_encode(id,c,ec);
    if (remain <= 0) {
        goto invalid;
    }
    c += ec - remain;
    ec = remain;

    remain = amf_encode(obj,c,ec);
    if (remain <= 0) {
        goto invalid;
    }

invalid:

    amf_free_data(obj);
    amf_free_data(id);
    amf_free_data(conn);

    return RTMP_FAILED;
}

/*play/publish stream*/
int rtmp_play(rtmp_t rtmp)
{
    return 0;
}

int rtmp_publish(rtmp_t rtmp)
{
    return 0;
}

/*send/recv packet from server*/
int rtmp_send(rtmp_t rtmp,const rtmp_packet_t *pkt)
{
    return 0;
}

int rtmp_recv(rtmp_t rtmp,rtmp_packet_t *pkt)
{
    return 0;
}


