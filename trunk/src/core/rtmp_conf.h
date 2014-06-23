
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __CONF_H_INCLUDED__
#define __CONF_H_INCLUDED__


#define GET_CONF_CURRENT           0
#define GET_CONF_NEXT              1
#define GET_CONF_CHILD             2

#define CONF_CONFIG_BLOCK_START    0
#define CONF_CONFIG_BLOCK_DONE     1
#define CONF_CONFIG_NEXT           2
#define CONF_CONFIG_ERROR          3
#define CONF_CONFIG_END            4

#define CONF_ON                    1
#define CONF_OFF                   0

typedef struct rtmp_conf_s rtmp_conf_t;
typedef struct rtmp_conf_tree_s rtmp_conf_tree_t;

struct rtmp_conf_s {
    void  *data;   /*void data*/

    array_t argv;

    link_t  v;   /*next: child ; prev : parent*/
    link_t  h;
};

struct rtmp_conf_tree_s {
    rtmp_conf_t *root;
};

/*parse file*/
int32_t rtmp_conf_parse(rtmp_cycle_t *cycle);
rtmp_conf_t *rtmp_get_conf(rtmp_conf_t *conf,char *name,int type);

#endif