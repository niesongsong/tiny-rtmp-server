
/*
 * CopyLeft (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

#define conn_app_set_string(key,member,len)                             \
    do {                                                                \
        amf_data_t *pr;                                                 \
        char       *val;                                                \
        pr = amf_get_prop(amf,key);                                     \
        if (pr) {                                                       \
            val = amf_get_string(pr);                                   \
            if (val == NULL) break;                                     \
            conn->member = (char*)mem_pcalloc(session->pool,len+1);     \
            if (conn->member) {                                         \
                strncpy(conn->member,val,len - 1);                      \
            }                                                           \
        }                                                               \
    }while(0)

#define conn_app_set_number(key,member)                                 \
    do {                                                                \
        amf_data_t *pr;                                                 \
        pr = amf_get_prop(amf,key);                                     \
        if (pr) {                                                       \
            conn->member = amf_get_number(pr);                          \
        }                                                               \
    }while(0)

static int32_t rtmp_amf_parse_connect(rtmp_session_t *session,amf_data_t *amf);

static int32_t rtmp_connect_success_send(rtmp_session_t *session,
    rtmp_chunk_stream_t *st);

static uint32_t rtmp_connect_amf_result(rtmp_session_t *sesssion,
    rtmp_chunk_stream_t *st);

static int32_t rtmp_connect_failed_send(rtmp_session_t *session,
    rtmp_chunk_stream_t *st);

int32_t rtmp_amf_cmd_connect(rtmp_session_t *session,
    rtmp_chunk_stream_t *st,amf_data_t *amf[],uint32_t num)
{
    int32_t             rc,i,h;
    double              transmitid;
    rtmp_addr_inet_t   *addr,*addrs;
    rtmp_addr_port_t   *port;
    struct sockaddr_in *addr_in;
    rtmp_host_t       **host;
    rtmp_app_t         *app;
    rtmp_session_connect_t *conn;

    if (num < 3) {
        rtmp_log(RTMP_LOG_ERR,"amf number[%d] error!",num);
        return RTMP_FAILED;
    }

    transmitid = amf_get_number(amf[1]);
    if (transmitid != 1.0) {
        rtmp_log(RTMP_LOG_ERR,"transmit id: %f",transmitid);
        return RTMP_FAILED;
    }

    rc = rtmp_amf_parse_connect(session,amf[2]);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_ERR,"[%d]rtmp_amf_parse_connect() failed!",
            session->sid);
        return RTMP_FAILED;
    }

    conn = session->conn;
    conn->trans = transmitid;

    addr = session->c->listening->data;
    if (addr->addr.sin_addr.s_addr == INADDR_ANY) {

        port = addr->port;
        addrs = port->addr_in.elts;
        addr_in = (struct sockaddr_in *)&session->c->local_sockaddr;

        for (i = port->addr_in.nelts - 1;i > 0;i--) {
            if (addrs[i].addr.sin_addr.s_addr == addr_in->sin_addr.s_addr) {
                break;
            }
        }
        addr = &addrs[i];
    }

    /*check vhost*/
    host = addr->hosts.elts;
    for (h = 0; h < (int32_t)addr->hosts.nelts;h++) {
        if (strcmp(host[h]->name,conn->vhost) == 0) {
            session->host_ctx = host[h];
            break;
        }

        if (host[h]->hconf->default_server) {
            session->host_ctx = host[h];
        }
    }

    if (session->host_ctx == 0) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]check vhost:\"%s\" not found!",
            session->sid,conn->vhost);
        return RTMP_FAILED;
    }
    
    rtmp_log(RTMP_LOG_INFO,"[%d]check vhost=\"%s\" found!",
        session->sid,session->host_ctx->name);

    /*find app*/
    session->app_ctx = rtmp_app_conf_find(conn->app,&session->host_ctx->apps);
    if (session->app_ctx == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]check app=\"%s\" not found!",
            session->sid,conn->app);

        /*send connect result*/
        (void)rtmp_connect_failed_send(session,st);
        return RTMP_FAILED;
    }
    app = session->app_ctx;

    rtmp_log(RTMP_LOG_INFO,"[%d]check app=\"%s\" args=\"%s\" found!",
        session->sid,conn->app,conn->args?conn->args:"(null)");

    session->chunk_pool = app->chunk_pool;
    if (app->conf->chunk_size) {
        session->out_chunk_size = app->conf->chunk_size;
    }

    return rtmp_connect_success_send(session,st);
}

int32_t rtmp_amf_parse_connect(rtmp_session_t *session,amf_data_t *amf)
{
    char                   *vhost,*ch;
    rtmp_session_connect_t *conn;

    if (session->conn != NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]connect app \"%s\" duplicate!",
            session->sid,session->conn->app);
        return RTMP_FAILED;
    }

    conn = mem_pcalloc(session->pool,sizeof(rtmp_session_connect_t));
    if (conn == NULL) {
        rtmp_log(RTMP_LOG_ERR,"[%d]connect app \"%s\" memory failed!",
            session->sid);
        return RTMP_FAILED;
    }

    conn_app_set_string("tcUrl",tc_url,RTMP_CONN_URL_SIZE_MAX);
    conn_app_set_string("app",app,RTMP_CONN_APPNAME_SIZE_MAX);
    conn_app_set_string("pageUrl",page_url,RTMP_CONN_URL_SIZE_MAX);
    conn_app_set_string("swfUrl",swf_url,RTMP_CONN_URL_SIZE_MAX);
    conn_app_set_string("pageUrl",page_url,RTMP_CONN_URL_SIZE_MAX);
    conn_app_set_string("flashVer",flashver,RTMP_CONN_VER_SIZE_MAX);

    if (!conn->app || !conn->tc_url) {
        return RTMP_FAILED;
    }

    conn_app_set_number("audioCodecs",acodecs);
    conn_app_set_number("videoCodecs",vcodecs);
    conn_app_set_number("objectEncoding",object_encoding);

    /*get host*/
    if (memcmp("rtmp://",conn->tc_url,7) != 0) {
        return RTMP_FAILED;
    }
    
    vhost = mem_dup_str(conn->tc_url+7,session->pool);
    strtok(vhost,":/");

    ch = vhost;
    for (ch = vhost;*ch;ch++) {
        if (*ch == ':' || *ch == '/') {
            *ch = 0;
            break;
        }
    }
    
    conn->vhost = vhost;
    ch = strchr(conn->app,'?');
    if (ch) {
        *ch++ = 0;
        conn->args = mem_pcalloc(session->pool,RTMP_CONN_ARGS_SIZE_MAX);
        if (conn->args) {
            strncpy(conn->args,ch,RTMP_CONN_ARGS_SIZE_MAX - 1);
        }
    }

    session->conn = conn;
    return RTMP_OK;
}

static int32_t rtmp_connect_success_send(rtmp_session_t *session,
    rtmp_chunk_stream_t *st)
{
    int32_t                 rc;
    rtmp_chunk_header_t    *h;

    h = &st->hdr;

    rc = rtmp_create_append_chain(session,rtmp_create_ack_size,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append ack windows "
            "message failed!",session->sid);
        return RTMP_FAILED;
    }

    rc = rtmp_create_append_chain(session,rtmp_create_peer_bandwidth_size,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append bandwidth "
            "message failed!",session->sid);
        return RTMP_FAILED;
    }

    rc = rtmp_create_append_chain(session,rtmp_create_set_chunk_size,h);
    if (rc != RTMP_OK) {
        rtmp_log(RTMP_LOG_WARNING,"[%d]append set chunk size "
            "message failed!",session->sid);
        return RTMP_FAILED;
    }

    if (rtmp_connect_amf_result(session,st) != RTMP_OK) {
        return RTMP_FAILED;
    }

    session->out_chunk = NULL;
    session->out_last = NULL;

    session->c->write->handler = rtmp_chain_send;
    rtmp_chain_send(session->c->write);

    return RTMP_OK;
}

static uint32_t rtmp_connect_amf_result(rtmp_session_t *session,
    rtmp_chunk_stream_t *st)
{
    amf_data_t      *amf[4],*ecma;
    mem_buf_t       *buf; 
    mem_buf_chain_t *chain;

    amf[0] = amf_new_string("_result",0);
    amf[1] = amf_new_number(1.0);
    amf[2] = amf_new_object();

    /*properties*/
    if (amf[2]) {
        amf_put_prop(amf[2],"fmsVer",
            amf_new_string("FMS/"rtmp_sig_fms_ver,0));

        amf_put_prop(amf[2],"capabilities",amf_new_number(255));
        amf_put_prop(amf[2],"mode",amf_new_number(1));
    }

    /*informations*/
    amf[3] = amf_new_object();
    if (amf[3]) {
        amf_put_prop(amf[3],"level",amf_new_string("status",0));
        amf_put_prop(amf[3],"code",
            amf_new_string("NetConnection.Connect.Success",0));

        amf_put_prop(amf[3],"description",
            amf_new_string("Connection succeeded.",0));

        amf_put_prop(amf[3],"objectEncoding",
            amf_new_number(session->conn->object_encoding));

        ecma = amf_new_ecma_array();
        if (ecma) {
            amf_put_prop(ecma,"version",
                amf_new_string(rtmp_sig_fms_ver,0));

            amf_put_prop(amf[3],"data",ecma);
        }
    }

    buf = rtmp_prepare_amf_buffer(session->temp_pool,amf,4);
    if (buf == NULL) {
        return RTMP_FAILED;
    }

    chain = rtmp_prepare_memssage_buf(session,&st->hdr,buf);
    if (chain == NULL) {
        rtmp_log(RTMP_LOG_WARNING,"prepare connect app message failed!");
        return RTMP_FAILED;
    }

    if (rtmp_append_message_chain(session,chain) == -1) {
        rtmp_log(RTMP_LOG_WARNING,"append connect app message failed!");
        return RTMP_FAILED;
    }

    return  0;
}

static int32_t rtmp_connect_failed_send(rtmp_session_t *session,
    rtmp_chunk_stream_t *st)
{
    return RTMP_OK;
}
