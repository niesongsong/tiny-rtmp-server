

/*
 * Copyright (C) nie950@gmail.com
 */


#include "rtmp_config.h"
#include "rtmp_core.h"

#define LF      10
#define CR      13

typedef struct rtmp_conf_buf_s rtmp_conf_buf_t;
struct rtmp_conf_buf_s {
    char        *start;
    char        *pos;
    char        *file;
    mem_pool_t  *pool;
    size_t       len;
    long         line;
};

static rtmp_conf_t* rtmp_conf_parse_file(char *file,mem_pool_t *pool,link_t *parent);

#ifdef HAVE_DEBUG

void rtmp_dump_conf(rtmp_conf_t *conf);
static void rtmp_dump_conf_core(link_t *link);

#endif

static long rtmp_read_n(FILE *fp,char *buf,long n)
{
    long r,rc;

    r = 0;
    while (n > 0) {
        rc = fread(buf+r,1,n,fp);

        if (rc <= 0) {
            break;
        }

        r += rc;
        n -= r;
    }

    return r;
}

/*
 * comes from nginx 1.5.4
 */
static int rtmp_conf_read_token(rtmp_conf_buf_t *block,rtmp_conf_t* conf)
{
    char ch,*start,*word,*src,*dst,**cmd;
    int sharp_comment,need_space,last_space;
    int start_line,len;
    int quoted,d_quoted,s_quoted,found;

    found = 0;
    need_space = 0;
    last_space = 1;
    sharp_comment = 0;
    quoted = 0;
    s_quoted = 0;
    d_quoted = 0;

    for (;;) {

        if ((size_t)(block->pos - block->start) == block->len) {
            if (conf->argv.nelts > 0 || !last_space) {
                rtmp_log(RTMP_LOG_ERR,"file:%s line:%d unexpected eof ",  \
                    block->file,block->line,ch);
                return CONF_CONFIG_ERROR;
            }

            return CONF_CONFIG_END;
        }

        ch = *block->pos++;

        if (ch == '\n') {
            block->line++;
            if (sharp_comment) {   
                sharp_comment = 0;
            }
        }

        if (sharp_comment) {
            continue;
        }

        if (quoted) {
            quoted = 0;
            continue;
        }

        if (need_space) {
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                last_space = 1;
                need_space = 0;
                continue;
            }

            if (ch == ';') {
                return CONF_CONFIG_NEXT;
            }

            if (ch == '{') {
                return CONF_CONFIG_BLOCK_START;
            }
            rtmp_log(RTMP_LOG_ERR,"file:%s line:%d unexpected '%c'",  \
                block->file,block->line,ch);

            return CONF_CONFIG_ERROR;
        }

        if (last_space) {
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                continue;
            }

            start = block->pos - 1;
            start_line = block->line;
            switch (ch) {

            case ';':
            case '{':
                if (conf->argv.nelts == 0) {

                    rtmp_log(RTMP_LOG_WARNING,"file:%s line:%d unexpected '%c'",  \
                        block->file,block->line,ch);

                    return CONF_CONFIG_ERROR;
                }

                if (ch == '{') {
                    return CONF_CONFIG_BLOCK_START;
                }

                return CONF_CONFIG_NEXT;

            case '}':
                if (conf->argv.nelts != 0) {

                    rtmp_log(RTMP_LOG_WARNING,"file:%s line:%d unexpected '%c'",  \
                        block->file,block->line,ch);

                    return CONF_CONFIG_ERROR;
                }

                return CONF_CONFIG_BLOCK_DONE;

            case '#':
                sharp_comment = 1;
                continue;

            case '\\':
                quoted = 1;
                last_space = 0;
                continue;

            case '"':
                start++;
                d_quoted = 1;
                last_space = 0;
                continue;

            case '\'':
                start++;
                s_quoted = 1;
                last_space = 0;
                continue;

            default:
                last_space = 0;
            }

        } else {
            if (ch == '{') {
                continue;
            }

            if (ch == '\\') {
                quoted = 1;
                continue;
            }

            if (d_quoted) {
                if (ch == '"') {
                    d_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (s_quoted) {
                if (ch == '\'') {
                    s_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (ch == ' ' || ch == '\t' || ch == CR || ch == LF
                || ch == ';' || ch == '{')
            {
                last_space = 1;
                found = 1;
            }

            if (found) {

                word = mem_palloc(block->pool,block->pos - start + 1);
                if (word == NULL) {
                    return CONF_CONFIG_ERROR;
                }

                for (dst = word, src = start, len = 0;
                    src < block->pos - 1;
                    len++)
                {
                    if (*src == '\\') {
                        switch (src[1]) {
                        case '"':
                        case '\'':
                        case '\\':
                            src++;
                            break;

                        case 't':
                            *dst++ = '\t';
                            src += 2;
                            continue;

                        case 'r':
                            *dst++ = '\r';
                            src += 2;
                            continue;

                        case 'n':
                            *dst++ = '\n';
                            src += 2;
                            continue;
                        }

                    }
                    *dst++ = *src++;
                }
                *dst = '\0';

                cmd = array_push(&conf->argv);
                if (cmd == NULL) {
                    return RTMP_ERROR;
                }

                *cmd = word;

                if (ch == ';') {
                    return CONF_CONFIG_NEXT;
                }

                if (ch == '{') {
                    return CONF_CONFIG_BLOCK_START;
                }

                found = 0;
            }
        }
    }
}

static 
rtmp_conf_t* rtmp_conf_parse_buf(rtmp_conf_buf_t *block, link_t *parent)
{
    mem_pool_t  *pool;
    int          rc;
    link_t      *head,*prev,root;
    rtmp_conf_t *conf,*include,*child;
    char       **word,*file,*p,*start;

    list_init(&root);
    pool = block->pool;

    for (;;) {
    
        conf = mem_pcalloc(pool,sizeof(rtmp_conf_t));
        if (conf == NULL) {
            return NULL;
        }

        list_init(&conf->h);

        array_init(&conf->argv,pool,10,sizeof(char*));
        rc = rtmp_conf_read_token(block,conf);

        switch (rc) {
        case CONF_CONFIG_NEXT:

            conf->v.prev = parent;
            list_insert_tail(&root,&conf->h);

            word = conf->argv.elts;
            if ((conf->argv.nelts > 1) && (strcmp(word[0],"include") == 0)) {

                p = start = block->file;
                while (*start) {
                    if ((*start == '/') || (*start == '\\')) {
                        p = start;
                    }
                    start++;
                }

                if ((*p == '/') || (*p == '\\')) {

                    file = mem_pcalloc(pool,strlen(block->file)+strlen(word[1])+1);
                    if (!file) {
                        return NULL;
                    }

                    memcpy(file,block->file,(p - block->file)+1);
                    strcat(file,word[1]);

                    include = rtmp_conf_parse_file(file,pool,parent);

                } else {

                    include = rtmp_conf_parse_file(word[1],pool,parent);
                }
                
                if (include) {
                    prev = include->h.prev;
                    do {
                        head = prev->next;

                        list_remove(head);
                        list_insert_tail(&root,head);

                    } while (head != prev);
                }
            }

            break;

        case CONF_CONFIG_BLOCK_START:

            conf->v.prev = parent;

            list_insert_tail(&root, &conf->h);
            child = rtmp_conf_parse_buf(block,&conf->v);

            if (child) {
                conf->v.next = &child->v;
            }

            break;

        case CONF_CONFIG_BLOCK_DONE:

            if (&root != root.next) {

                conf = struct_entry(root.next,rtmp_conf_t,h);
                list_remove(&root);

                return conf;
            }

            return NULL;

        case CONF_CONFIG_END:

            if (&root != root.next) {
                conf = struct_entry(root.next,rtmp_conf_t,h);
                list_remove(&root);

                return conf;
            }

            return NULL;

        case CONF_CONFIG_ERROR:
            return NULL;

        default:
            break;
        }
    }

    return NULL;
}

rtmp_conf_t* rtmp_conf_parse_file(char *file,mem_pool_t *pool,link_t *parent)
{
    FILE           *fp;
    long            size,r;
    char           *buf;
    rtmp_conf_t    *conf;
    rtmp_conf_buf_t block;

    conf = 0;
    fp = fopen(file,"rb");
    if (fp == NULL) {
        rtmp_log(RTMP_LOG_ERR,"file:%s open failed!",file);
        return conf;
    }

    fseek(fp,0,SEEK_END);
    size = ftell(fp);

    if (size <= 0) {
        goto done_close;
    }

    buf = mem_malloc(size+1);
    if (buf == NULL) {
        goto done_close;
    }

    rewind(fp);

    r = rtmp_read_n(fp,buf,size);
    if (r < size) {
        goto done_free;
    }
    buf[size] = '\0';

    block.line = 0;
    block.pool = pool;
    block.file = file;
    block.pos = block.start = buf;
    block.len = size;

    conf = rtmp_conf_parse_buf(&block,parent);

done_free:
    mem_free(buf);

done_close:
    fclose(fp);

    return conf;
}

#ifdef HAVE_DEBUG

static void rtmp_dump_conf_core(link_t *link)
{
    link_t *next,*prev;
    rtmp_conf_t *conf,*pconf,*c;
    char ** word,**pword;
    uint32_t n;

    next = link;

    do {
        conf = struct_entry(next,rtmp_conf_t,h);
        word = conf->argv.elts;

        n = 0;
        while (n < conf->argv.nelts) {
            printf("(%s)",word[n]);
            n++;
        }

        prev = conf->v.prev;
        while (prev) {
            pconf = struct_entry(prev,rtmp_conf_t,v);
            pword = pconf->argv.elts;
            printf("->%s ",*pword);
            prev = pconf->v.prev;
        }
        printf("\n");

        /*dump child*/
        if (conf->v.next) {
            c = struct_entry(conf->v.next,rtmp_conf_t,v);
            rtmp_dump_conf_core(&c->h);
        }

        next = next->next;

    } while (next != link);
}

void rtmp_dump_conf(rtmp_conf_t *conf)
{
    rtmp_dump_conf_core(&conf->h);
}


#endif

int32_t rtmp_conf_parse(rtmp_cycle_t *cycle)
{
    rtmp_conf_t *conf;

    cycle->conf = NULL;

    conf = rtmp_conf_parse_file(cycle->conf_file,cycle->pool,0);

    if (conf) {
        cycle->conf = conf;

#if 0
        rtmp_dump_conf(conf);
#endif

        return RTMP_OK;
    }

    return RTMP_ERROR;
}

rtmp_conf_t *rtmp_get_conf(rtmp_conf_t *conf,char *name,int type)
{
    link_t       *next,*head;
    rtmp_conf_t  *pconf;
    char        **word;

    if (conf == NULL || name == NULL) {
        return NULL;
    }

    if ((type == GET_CONF_NEXT) || (type == GET_CONF_CURRENT)) {
        head = & conf->h;

    } else if ((type == GET_CONF_CHILD) && (conf->v.next)) {
        head = & (struct_entry(conf->v.next,rtmp_conf_t,v))->h;

    } else {
        return NULL;
    }

    next = head;
    if (type == GET_CONF_NEXT) {

        next = next->next;
        if (next == head) {
            return NULL;
        }
    }

    do {
        
        pconf = struct_entry(next,rtmp_conf_t,h);
        word = pconf->argv.elts;

        if (strcmp(*word,name) == 0) {
            return pconf;
        }

        next = next->next;
    } while (next != head);

    return NULL;
}
