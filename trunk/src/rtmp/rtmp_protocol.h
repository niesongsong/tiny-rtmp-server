

/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __RTMP_PROTOCOL_H_INCLUDED__
#define __RTMP_PROTOCOL_H_INCLUDED__

/*
 *define protocol const value
 */

/*

C -----> C0  C1 ----> S
| <----- S0  S1 <---- |
| <-----   S2   <-----|
| ----->   C2   ----> | 

*/

#define RTMP_HANDSHAKE_FAILED               0  /*handshake failed*/

#define RTMP_HANDSHAKE_SERVER_INIT          1  /*prepare*/
#define RTMP_HANDSHAKE_SERVER_C0C1          2   /*recv c0c1*/
#define RTMP_HANDSHAKE_SERVER_S0S1          3   /*send s0s1*/
#define RTMP_HANDSHAKE_SERVER_S2            4   /*send s2*/
#define RTMP_HANDSHAKE_SERVER_C2            5   /*recv c2*/
#define RTMP_HANDSHAKE_SERVER_DONE          6

#define RTMP_HANDSHAKE_CLIENT_INIT          7  /*prepare c0c1*/
#define RTMP_HANDSHAKE_CLIENT_C0C1          8  /*send c0c1*/
#define RTMP_HANDSHAKE_CLIENT_S0S1          9  /*recv s0s1*/
#define RTMP_HANDSHAKE_CLIENT_S2            10  /*recv s2*/
#define RTMP_HANDSHAKE_CLIENT_C2            11  /*send c2*/
#define RTMP_HANDSHAKE_CLIENT_DONE          12


#define rtmp_sig_fms_ver                    "4,5,6,5012"
#define rtmp_sig_amf0_ver                   0
#define rtmp_sig_client_id                  "ASAICiss"

#define rtmp_status_level                   "level"
#define rtmp_status_code                    "code"
#define rtmp_status_desc                    "description"
#define rtmp_status_details                 "details"
#define rtmp_status_clientid                "clientid"


#define rtmp_status_level_status            "status"
#define rtmp_status_level_error             "error"

#define rtmp_status_code_conn_success       "NetConnection.Connect.Success"
#define rtmp_status_code_conn_rejected      "NetConnection.Connect.Rejected"

#endif
