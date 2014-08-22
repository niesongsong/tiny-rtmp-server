
/*
 * CopyLeft (C) nie950@gmail.com
 */

#ifndef __RTMP_DEF_H_INCLUDED__
#define __RTMP_DEF_H_INCLUDED__

#define rtmp_abs(value)       (((value) >= 0) ? (value) : - (value))
#define rtmp_max(val1, val2)  (((val1) < (val2)) ? (val2) : (val1))
#define rtmp_min(val1, val2)  (((val1) > (val2)) ? (val2) : (val1))

#define rtmp_array_size(x)    (sizeof(x)/sizeof((x)[0]))
#define rtmp_hash(key, c)     ((uint32_t) key * 31 + c)

#define rtmp_invaild_ptr      ((void*)(-1))

typedef struct rtmp_cycle_s         rtmp_cycle_t;
typedef struct rtmp_module_s        rtmp_module_t;
typedef struct rtmp_listening_s     rtmp_listening_t;
typedef struct rtmp_connection_s    rtmp_connection_t;
typedef struct rtmp_session_s       rtmp_session_t;
typedef struct rtmp_addr_port_s     rtmp_addr_port_t;
typedef struct rtmp_addr_inet_s     rtmp_addr_inet_t;

#endif
