
/*
 * Copyright (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

#ifdef HAVE_DEBUG
extern void rtmp_core_dump_listennings(rtmp_cycle_t *cycle);
#endif

int32_t rtmp_host_conf_init(rtmp_cycle_t *c, rtmp_conf_t *conf,
    rtmp_host_t *host)
{
    uint16_t            sin_port;
    char                url[64],**word,*colon;
    rtmp_conf_t        *it,*head;
    struct sockaddr_in  temp;
    int32_t             i,n;
    void              **data;
    rtmp_addr_port_t   *port;
    rtmp_addr_inet_t   *addr;

    head = it = rtmp_get_conf(conf,"listen",GET_CONF_NEXT);

    do {
        strcpy(url,"0.0.0.0");
        sin_port = RTMP_DEFAULT_PORT;

        if (it == NULL) {
            goto found;
        }

        if (it->argv.nelts <= 1) {
            rtmp_log(RTMP_LOG_ERR,"listen conf error");
            return RTMP_FAILED;
        }

        word = it->argv.elts;
        strncpy(url,word[1],sizeof(url)-1);

        colon = strchr(url,':');
        if (colon) {
            sin_port = (uint16_t)atol(colon+1);
            *colon = 0;
        } else {
            sin_port = (uint16_t)atol(url);
            strcpy(url,"0.0.0.0");
        }

found:  
	memset(&temp,0,sizeof(temp));

        temp.sin_family = AF_INET;
        temp.sin_port = htons(sin_port);
        temp.sin_addr.s_addr = inet_addr(url);

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
            array_init(&addr->hosts,c->pool,5,sizeof(void**));

            data = array_push(&addr->hosts);
            *data = host;

        } else {

            if (temp.sin_addr.s_addr == addr->addr.sin_addr.s_addr) {

                data = addr->hosts.elts;
                for (i = 0;i < (int32_t)addr->hosts.nelts;i++) {
                    if (data[i] == host) {
                        rtmp_log(RTMP_LOG_WARNING,"duplicate listening");
                        break;
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

                array_init(&addr->hosts,c->pool,5,sizeof(void**));

                data = array_push(&addr->hosts);
                *data = host;
            }
        }

        it = rtmp_get_conf(it,"listen",GET_CONF_NEXT);
    } while ((it != NULL) && (it != head));

    return RTMP_OK;
}

int32_t rtmp_app_conf_init(rtmp_cycle_t *c, rtmp_conf_t *conf,
    rtmp_app_conf_t *app)
{
    return RTMP_OK;
}
