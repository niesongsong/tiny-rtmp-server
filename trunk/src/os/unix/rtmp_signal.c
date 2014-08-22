

/*
 * CopyLeft (C) nie950@gmail.com
 */

#include "rtmp_config.h"
#include "rtmp_core.h"

typedef struct {
    int     signo;
    char   *signame;
    char   *name;
    void  (*handler)(int signo);
}rtmp_signal_t;

void rtmp_signal_handler(int signo);

rtmp_signal_t signals[] = {
    { SIGALRM, "SIGALRM", "", rtmp_signal_handler },
    { SIGINT,  "SIGINT", "", rtmp_signal_handler },
    { SIGIO,   "SIGIO", "", rtmp_signal_handler },
    { SIGCHLD, "SIGCHLD", "", rtmp_signal_handler },
    { SIGSYS,  "SIGSYS, SIG_IGN", "", SIG_IGN },
    { SIGPIPE, "SIGPIPE, SIG_IGN", "", SIG_IGN },
    { 0, NULL, "", NULL }
};

int rtmp_init_signals()
{
    rtmp_signal_t      *sig;
    struct sigaction    sa;

    for (sig = signals; sig->signo != 0; sig++) {
        memset(&sa, sizeof(struct sigaction),0);
        sa.sa_handler = sig->handler;
        sigemptyset(&sa.sa_mask);
        if (sigaction(sig->signo, &sa, NULL) == -1) {
            return -1;
        }
    }

    return 0;
}

void rtmp_signal_handler(int signo)
{
    return ;
}