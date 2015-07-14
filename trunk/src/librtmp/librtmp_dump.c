
#include "librtmp_in.h"

void *my_malloc(size_t sz,void *u) 
{
    return malloc(sz);
}

void my_free(void *p,void *u)
{
    if (p != NULL) {
        free(p);
    }
}

int main(int argc,char **argv)
{
    rtmp_net_conn_t     conn;
    rtmp_net_stream_t   stream;
    int                 rc;
    rtmp_message_t       msg;
    

#ifdef WIN32
    WSADATA             wsadata;

    rc = WSAStartup(MAKEWORD(2,2),&wsadata);

    if (rc != 0) {
        return -2;
    }

#endif

    rtmp_time_update();

    rtmp_log_init(RTMP_LOG_DEBUG,"rtmp_core.log");

    /*NetConnection()*/
    conn = rtmp_conn_create(my_malloc,my_free,NULL);
    if (conn == NULL) {
        rtmp_log(RTMP_LOG_ERR,"NetConnection() failed!");
        return -1;
    }

    /*NetConnection.connect()*/
    rc = rtmp_conn_connect(conn,"rtmp://liveteach.xescdn.com/live_server");
    if (rc != RTMP_ERR_OK) {
        rtmp_log(RTMP_LOG_ERR,"NetConnection.connect() failed:[%d]",rc);
        return -1;
    }

    /*NetStream()*/
    stream = rtmp_stream_create(conn,"mux_4");
    if (stream == NULL) {
        rtmp_log(RTMP_LOG_ERR,"NetStream() failed!");
        return -1;
    }

    /*NetStream.play()*/
    rc = rtmp_stream_play(stream);
    if (rc != RTMP_ERR_OK) {
        rtmp_log(RTMP_LOG_ERR,"NetStream.play() failed:[%d]",rc);
        return -1;
    }

    while (TRUE) {
        rc = rtmp_stream_recv(stream,&msg);

        if (rc == RTMP_ERR_OK) {

            rtmp_log(RTMP_LOG_DEBUG,"recv packet : "
                "timestamp:[%d] type_id:[%d] length:[%d]",
                msg.timestamp,msg.type,msg.body_size);

        }

        if (rc == RTMP_ERR_FAILED) {
            break;
        }
    }

    rtmp_stream_close(stream);
    rtmp_conn_destroy(conn);

    return 0;
}

