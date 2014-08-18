
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

#ifdef HAVE_DEBUG
extern void rtmp_core_dump_listennings(rtmp_cycle_t *cycle);
extern void rtmp_core_dump_ports(rtmp_cycle_t *cycle);
#endif

typedef int32_t (*rtmp_host_conf_pt)(rtmp_conf_t *conf,rtmp_host_t *host);

typedef struct rtmp_host_conf_handle_s{
    char               *name;
    rtmp_host_conf_pt   pt;
}rtmp_host_conf_handle_t;

static int32_t rtmp_parse_ipmask(char *ipmask,rtmp_ip_table_t *ipt);

static int32_t rtmp_host_servername_block(rtmp_conf_t *conf,rtmp_host_t *host);
static int32_t rtmp_host_listen_block(rtmp_conf_t *conf,rtmp_host_t *host);
static int32_t rtmp_host_deny_block(rtmp_conf_t *conf,rtmp_host_t *host);
static int32_t rtmp_host_allow_block(rtmp_conf_t *conf,rtmp_host_t *host);
static int32_t rtmp_host_ping_block(rtmp_conf_t *conf,rtmp_host_t *host);
static int32_t rtmp_host_chunksize_block(rtmp_conf_t *conf,rtmp_host_t *host);
static int32_t rtmp_host_app_block(rtmp_conf_t *conf,rtmp_host_t *host);

static int32_t rtmp_host_listen_add(rtmp_host_t *host,
    const char *ip,uint16_t sin_port,int default_server);


int32_t rtmp_app_conf_block(rtmp_conf_t *conf,rtmp_app_t *app);

static rtmp_host_conf_handle_t rtmp_host_conf_map[] = {
    {"listen",      rtmp_host_listen_block},
    {"deny",        rtmp_host_deny_block},
    {"allow",       rtmp_host_allow_block},
    {"ping",        rtmp_host_ping_block},
    {"chunk_size",  rtmp_host_chunksize_block},
    {"chunk_size",  rtmp_host_chunksize_block},
    {"app",         rtmp_host_app_block},
    {"server_name", rtmp_host_servername_block},
};

int32_t rtmp_host_conf_block(rtmp_cycle_t *c,rtmp_conf_t *conf)
{
    rtmp_conf_t         *it,*h;
    rtmp_host_t         *host;
    link_t              *next;
    rtmp_host_conf_t    *hconf;
    char               **word;
    uint32_t             i;
    int32_t              rc;

    if (conf->v.next == NULL) {
        return RTMP_FAILED;
    }
    h = struct_entry(conf->v.next,rtmp_conf_t,v);

    host = mem_pcalloc(c->pool,sizeof(rtmp_host_t) 
        + sizeof(rtmp_host_conf_t));
    if (host == NULL) {
        return RTMP_FAILED;
    }
    hconf = (rtmp_host_conf_t *)(&host[1]);

    host->hconf = hconf;
    host->cycle = c;

    for (i = 0;i < rtmp_array_size(hconf->allow_list);i++) {
        list_init(&hconf->allow_list[i]);
    }

    for (i = 0;i < rtmp_array_size(hconf->deny_list);i++) {
        list_init(&hconf->deny_list[i]);
    }

    strcpy(host->name,RTMP_HOSTNAME_DEF);
    rc = array_init(& host->apps,c->pool,10,sizeof(void**));
    if (rc != RTMP_OK) {
        return RTMP_FAILED;
    }
    
    rc = RTMP_OK;
    next = &h->h;
    do {
        it = struct_entry(next,rtmp_conf_t,h);
        if (it->argv.nelts < 1) {
            continue;
        }
        word = it->argv.elts;

        for (i = 0;i < rtmp_array_size(rtmp_host_conf_map);i++) {
            if (strcmp(word[0],rtmp_host_conf_map[i].name) == 0) {
                rc = rtmp_host_conf_map[i].pt(it,host);
                if (rc == RTMP_FAILED) {
                    break;
                }
            }
        }
        next = next->next;
    } while (next != & h->h);

    return rc;
}

static int32_t rtmp_host_listen_block(rtmp_conf_t *conf,rtmp_host_t *host)
{
    uint16_t            sin_port;
    uint32_t            default_server;
    char                url[64],**word,*colon;

    strcpy(url,"0.0.0.0");
    sin_port = RTMP_DEFAULT_PORT;

    if (conf->argv.nelts <= 1) {
        rtmp_log(RTMP_LOG_ERR,"listen conf error");
        return RTMP_FAILED;
    }

    word = conf->argv.elts;
    strncpy(url,word[1],sizeof(url)-1);

    default_server = 0;
    if (conf->argv.nelts > 2) {
        if (strcmp(word[2],"default") == 0) {
            default_server = 1;
        }
    }

    colon = strchr(url,':');
    if (colon) {
        sin_port = (uint16_t)atol(colon+1);
        *colon = 0;
    } else {
        sin_port = (uint16_t)atol(url);
        strcpy(url,"0.0.0.0");
    }

    return rtmp_host_listen_add(host,url,sin_port,default_server);
}

static int32_t rtmp_host_listen_add(rtmp_host_t *host,
    const char *ip,uint16_t sin_port,int default_server)
{
    struct sockaddr_in  temp;
    rtmp_addr_port_t   *port;
    rtmp_addr_inet_t   *addr;
    int32_t             i,n;
    void              **data;
    rtmp_cycle_t       *c;

    c = host->cycle;
    memset(&temp,0,sizeof(temp));

    temp.sin_family = AF_INET;
    temp.sin_port = htons(sin_port);
    temp.sin_addr.s_addr = inet_addr(ip);

    /*find port*/
    port = (rtmp_addr_port_t *)c->ports.elts;
    for (i = 0;i < (int32_t)c->ports.nelts; i++) {

        if ((port[i].family == temp.sin_family) 
            && (port[i].port == temp.sin_port)) {
                port = & port[i];
                break; 
        }
    }

    if (i == (int32_t)c->ports.nelts) {
        port = array_push(&c->ports);

        port->port = temp.sin_port;
        port->family = temp.sin_family;

        array_init(&port->addr_in,c->pool,5,sizeof(rtmp_addr_inet_t));
    }

    /*find addr*/
    addr = port->addr_in.elts;
    for (i = 0;i < (int32_t)port->addr_in.nelts;i++) {
        if (temp.sin_addr.s_addr <= addr->addr.sin_addr.s_addr) {
            break;
        }
    }

    if (i == (int32_t)port->addr_in.nelts) {
        addr = array_push(&port->addr_in);

        addr->addr = temp;
        addr->port = port;

        array_init(&addr->hosts,c->pool,5,sizeof(void**));

        data = array_push(&addr->hosts);
        *data = host;

    } else {

        if (temp.sin_addr.s_addr == addr->addr.sin_addr.s_addr) {

            data = addr->hosts.elts;
            for (i = 0;i < (int32_t)addr->hosts.nelts;i++) {
                if (data[i] == host) {
                    rtmp_log(RTMP_LOG_WARNING,"duplicate listening");
                    return RTMP_IGNORE;
                }
            }

            if (i == (int32_t)addr->hosts.nelts) {
                data = array_push(&addr->hosts);
                *data = host;
            }

        } else {
            array_push(&port->addr_in);

            addr = port->addr_in.elts;

            for (n = (int32_t)(port->addr_in.nelts - 1) ;n > i;n--) {
                addr[n] = addr[n - 1];
            }

            addr[i].addr = temp;
            addr[i].port = port;

            array_init(&addr->hosts,c->pool,5,sizeof(void**));

            data = array_push(&addr->hosts);
            *data = host;
        }
    }

    host->hconf->default_server = default_server;
    return RTMP_OK;
}

static int32_t rtmp_host_servername_block(rtmp_conf_t *conf,rtmp_host_t *host)
{
    rtmp_cycle_t      *c;
    rtmp_host_t      **vhost;
    char             **word;
    
    word = conf->argv.elts;
    if (conf->argv.nelts < 2) {
        return RTMP_FAILED;
    }

    c = host->cycle;
    if (rtmp_host_conf_find(word[1],& c->server_list)) {
        rtmp_log(RTMP_LOG_WARNING,"server [%s] duplicate!",word[1]);
        return RTMP_IGNORE;
    }

    vhost = array_push(& c->server_list);
    if (!vhost) {
        rtmp_log(RTMP_LOG_ERR,"array:no more room!");
        return RTMP_FAILED;
    }
    strncpy(host->name,word[1],sizeof(host->name)-1);
    *vhost = host;

    return RTMP_OK;
}

static int32_t rtmp_host_deny_block(rtmp_conf_t *conf,rtmp_host_t *host)
{
    rtmp_ip_table_t *ipt;
    rtmp_cycle_t     *c;
    char            **word;
    int32_t           rc;

    c = host->cycle;
    word = conf->argv.elts;
    if (conf->argv.nelts != 3) {
        rtmp_log(RTMP_LOG_ERR,"argv number error!");
        return RTMP_FAILED;
    }

    ipt = mem_palloc(c->pool,sizeof(rtmp_ip_table_t));

    if (strcmp(word[2],"all") == 0) {
        return RTMP_OK;
    }

    rc = rtmp_parse_ipmask(word[2],ipt);
    if ((rc == RTMP_IGNORE) || (rc == RTMP_OK)) {
        return RTMP_OK;
    }

    return RTMP_OK;
}

static int32_t rtmp_host_allow_block(rtmp_conf_t *conf,rtmp_host_t *host)
{
    return RTMP_OK;
}

static int32_t rtmp_host_ping_block(rtmp_conf_t *conf,rtmp_host_t *host)
{
    return RTMP_OK;
}

static int32_t rtmp_host_chunksize_block(rtmp_conf_t *conf,rtmp_host_t *host)
{
    return RTMP_OK;
}

static int32_t rtmp_host_app_block(rtmp_conf_t *conf,rtmp_host_t *host)
{
    rtmp_app_t       **app,*app_temp;
    char              *appname,**word;

    appname = RTMP_APPNAME_DEF;
    if (conf->argv.nelts > 1) {
        word = conf->argv.elts;
        appname = word[1];
    }

    /*find app*/
    if (rtmp_app_conf_find(appname,&host->apps)) {
        rtmp_log(RTMP_LOG_WARNING," %s::%s duplicate!",
            host->name,appname);
    }

    app_temp = rtmp_create_app(host->cycle->pool,host);
    if (app_temp == NULL) {
        rtmp_log(RTMP_LOG_ERR,"alloc app failed!");
        return RTMP_FAILED;
    }
    strncpy(app_temp->name,appname,sizeof(app_temp->name)-1);

    app = array_push(& host->apps);
    if (app == NULL) {
        rtmp_log(RTMP_LOG_ERR,"array no more room!");
        mem_destroy_pool(app_temp->chunk_pool);
        return RTMP_FAILED;
    }
    *app = app_temp;
    
    if (conf->argv.nelts >= 2) {
        return rtmp_app_conf_block(conf,*app);
    }
    return RTMP_OK;
}

int32_t rtmp_app_conf_block(rtmp_conf_t *conf,rtmp_app_t *app)
{
    rtmp_app_conf_t     *aconf;
    rtmp_host_t         *host;
    uint32_t             size,i;

    aconf = app->conf;
    aconf->stream_buckets = 1024;

    host = app->host;

    size = sizeof(list_t) *aconf->stream_buckets;
    app->lives = mem_palloc(host->cycle->pool,size);

    if (app->lives == NULL) {
        rtmp_log(RTMP_LOG_ERR,"alloc lives' ptr failed!");
        return RTMP_FAILED;
    }

    for (i = 0;i < aconf->stream_buckets;i++) {
        list_init(& app->lives[i]);
    }

    return RTMP_OK;
}

int32_t rtmp_parse_ipmask(char *ipmask,rtmp_ip_table_t *ipt)
{
    return RTMP_OK;
}