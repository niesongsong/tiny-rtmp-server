
/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

int rtmp_show_version;
int rtmp_show_help;
int rtmp_test_config;
int rtmp_sigal_stop;
int rtmp_sigal_exit;
int rtmp_sigal_reload;
int rtmp_daemon_mode;

char* rtmp_conf_file = "conf/rtmpd.conf";
char* rtmp_log_file = "logs/rtmpd.log";
char* rtmp_pid_file = "rtmpd.pid";
char* rtmp_lock_file = "rtmpd.lock";
char* rtmp_prefix_path = "../";

rtmp_module_t *rtmp_modules[] = {
    &rtmp_core_moudle,
    &rtmp_event_module,
    &rtmp_host_moudle,
    NULL
};

extern uint32_t  mem_pagesize;
extern uint32_t  mem_pagesize_shift;
extern uint32_t  mem_cacheline_size;

uint32_t rtmp_max_modules;
static void rtmp_run_workers_cycle(rtmp_cycle_t * cycle);
static void rtmp_run_master_cycle(rtmp_cycle_t * cycle);
static void rtmp_run_master(rtmp_cycle_t * cycle);
static uint32_t rtmp_os_init(void);

static uint32_t rtmp_os_init(void)
{
#ifdef HAVE_OS_WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
        rtmp_log(RTMP_LOG_ERR,"WSAStartup() [%d] failed!", sock_errno);
        return RTMP_FAILED;
    }
    return RTMP_OK;
#else

    if (rtmp_init_signals() != 0) {
        return RTMP_FAILED;
    }

    return RTMP_OK;
#endif
}

static int
rtmp_get_options(int argc, char *const *argv)
{
    char     *p;
    int      i;

    for (i = 1; i < argc; i++) {

        p = argv[i];

        if (*p++ != '-') {
            return RTMP_ERROR;
        }

        while (*p) {

            switch (*p++) {

            case '?':
            case 'h':
                rtmp_show_version = 1;
                rtmp_show_help = 1;
                break;

            case 'l':
                if (*p) {
                    rtmp_log_file = p;
                    continue;
                }

                if (argv[++i]) {
                    rtmp_log_file = argv[i];
                    continue;
                }

                rtmp_log(RTMP_LOG_ERR, "option \"-l\" requires file name");
                return RTMP_ERROR;

            case 'c':
                if (*p) {
                    rtmp_conf_file = p;
                    continue;
                }

                if (argv[++i]) {
                    rtmp_conf_file = argv[i];
                    continue;
                }

                rtmp_log(RTMP_LOG_ERR, "option \"-c\" requires file name");
                return RTMP_ERROR;

            case 'v':
            case 'V':
                rtmp_show_version = 1;
                break;

            case 't':
                rtmp_test_config = 1;
                break;

            case 's':
                if (*p == 0) {
                    p = argv[i];
                }

                if (strcmp(p,"stop") == 0) {
                    rtmp_sigal_stop = 1;
                }

                if (strcmp(p,"reload") == 0) {
                    rtmp_sigal_reload = 1;
                }

                rtmp_log(RTMP_LOG_ERR, "option \"-s\" requires parameter");
                return RTMP_ERROR;

            default:
                rtmp_log(RTMP_LOG_ERR, "invalid option: \"%c\"", *(p - 1));
                return RTMP_ERROR;

            }
        }
    }

    return RTMP_OK;
}

void rtmp_do_test();

int main(int argc,char **argv)
{
    rtmp_cycle_t *rtmp_cycle;

    /*os init*/
    if (rtmp_os_init() != RTMP_OK) {
        exit(-1);
    }

    if (rtmp_strerror_init() != RTMP_OK) {
        exit(-1);
    }

    rtmp_time_update();

    if (rtmp_log_init(RTMP_LOG_DEBUG,rtmp_log_file) != RTMP_OK) {
        exit(-1);
    }

    if (rtmp_get_options(argc,argv) != RTMP_OK) {
        exit(-1);
    }

    if (rtmp_show_version) {
        rtmp_log(RTMP_LOG_INFO,"");

        if (rtmp_show_help) {
            rtmp_log(RTMP_LOG_INFO,"");
        }

        if (!rtmp_test_config) {
            return 0;
        }
    }

    /*init cycle*/
    rtmp_cycle = rtmp_init_cycle();

    if (rtmp_test_config) {

        rtmp_log(RTMP_LOG_INFO,"test file [%s] %s!",  \
            rtmp_conf_file,                           \
            (rtmp_cycle)?"ok":"failed");

        return 0;
    }

    if (rtmp_cycle == NULL) {
        rtmp_log(RTMP_LOG_ERR,"rtmp_init_cycle() failed!");
        return RTMP_FAILED;
    }

    /*run cycle*/
    rtmp_run_cycle(rtmp_cycle);

    return 0;
}

rtmp_cycle_t* rtmp_init_cycle(void)
{
    rtmp_cycle_t  *cycle;
    mem_pool_t    *pool,*temp_pool;
    size_t         slen;
    int            m;
    rtmp_module_t *module;

    mem_pagesize = 4096;
    mem_pagesize_shift = 12;
    mem_cacheline_size = 4096;

    pool = mem_create_pool(MEM_DEFAULT_POOL_SIZE);
    if (pool == NULL) {
        rtmp_log(RTMP_LOG_ERR,"alloc pool failed!");
        return NULL;
    }

    temp_pool = mem_create_pool(MEM_DEFAULT_POOL_SIZE);
    if (temp_pool == NULL) {
        rtmp_log(RTMP_LOG_ERR,"alloc temp_pool failed!");
        return NULL;
    }

    slen = strlen(rtmp_conf_file);
    cycle = mem_palloc(pool,sizeof(rtmp_cycle_t));
    if (cycle == NULL) {
        return NULL;
    }

    cycle->pool = pool;
    cycle->temp_pool = temp_pool;

    cycle->conf_file = mem_pcalloc(pool,slen + 1);
    if (cycle->conf_file == NULL) {
        return NULL;
    }
    memcpy(cycle->conf_file,rtmp_conf_file,slen);

    rtmp_max_modules = 0;
    for (m = 0;rtmp_modules[m];m++) {
        rtmp_modules[m]->index = m;
        rtmp_max_modules++;
    }
    cycle->conf = mem_pcalloc(pool,sizeof(void*));

    /*create modules*/
    for (m = 0;rtmp_modules[m];m++) {

        module = rtmp_modules[m];
        if (module->create_module != NULL) {
            module->ctx = module->create_module(cycle);
            if (module->ctx == NULL) {
                rtmp_log(RTMP_LOG_WARNING,"create module failed[%d]",m);
                return NULL;
            }
        }
    }

    if (rtmp_conf_parse(cycle) != RTMP_OK) {
        return NULL;
    }

    for (m = 0;rtmp_modules[m];m++) {

        module = rtmp_modules[m];
        if (module->init_cycle != NULL) {
            if (module->init_cycle(cycle,module) != RTMP_OK) {
                rtmp_log(RTMP_LOG_WARNING,"configure module failed[%d]",m);
                return 0;
            }
        }
    }

    /*init core process*/
    module = rtmp_modules[0];
    if (module->init_forking) {
        if (rtmp_modules[0]->init_forking(cycle,module) == RTMP_FAILED) {
            return 0;
        }
    }

    return cycle;
}

void rtmp_run_cycle(rtmp_cycle_t * cycle)
{
    if (cycle->daemon == CONF_ON) {
        rtmp_run_master_cycle(cycle);
    } else {
        rtmp_run_workers_cycle(cycle);
    }

    return ;
}

static void rtmp_run_workers_cycle(rtmp_cycle_t * cycle)
{
    uint32_t m;
    rtmp_module_t *module;

    for (m = 1;m < rtmp_max_modules;m++) {

        module = rtmp_modules[m];
        if (module->init_forking != NULL) {
            if (module->init_forking(cycle,module) != RTMP_OK) {
                rtmp_log(RTMP_LOG_WARNING,"forked failed[%d]",m);
                return;
            }
        }
    }

    for (;;) {
        rtmp_event_poll(80);
        rtmp_time_update();
    }

    return;
}

static void rtmp_run_master_cycle(rtmp_cycle_t * cycle)
{
    uint32_t i;
    pid_t pid;

    for (i = 0;i < cycle->workers; i++) {
        pid = fork();

        if (pid < 0) {
            exit(-1);
        }

        if (pid == 0) {
            rtmp_run_workers_cycle(cycle);
            exit(0);
        }
    }

    rtmp_run_master(cycle);

    return;
}

static void rtmp_run_master(rtmp_cycle_t * cycle)
{
    for (;;) {

    }
    return ;
}
