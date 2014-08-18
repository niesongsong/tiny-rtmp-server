
/*
 * Copyright (C) nie950@gmail.com
 */

#ifndef __AMF_H_INCLUDED__
#define __AMF_H_INCLUDED__

#define SCRIPT_AMF0                 0
#define SCRIPT_AMF3                 3

/*AMF0 data types*/

#define amf_number                  0x00
#define amf_boolean                 0x01
#define amf_string                  0x02
#define amf_object                  0x03
#define amf_movieclip               0x04 /* reserved */
#define amf_null                    0x05
#define amf_undefined               0x06
#define amf_reference               0x07
#define amf_ecma_array              0x08
#define amf_object_end              0x09
#define amf_strict_array            0x0a
#define amf_date                    0x0b
#define amf_long_string             0x0c
#define amf_unsupported             0x0d
#define amf_recordset               0x0e /* reserved */
#define amf_xml_document            0x0f
#define amf_typed_object            0x10
#define amf_avmplus_object          0x11
#define amf_invalid                 0xff

/*AMF3 data types*/

#define amf3_null                   0x01
#define amf3_false                  0x02
#define amf3_true                   0x03
#define amf3_integer                0x04
#define amf3_double                 0x05
#define amf3_string                 0x06
#define amf3_xml_doc                0x07
#define amf3_date                   0x08
#define amf3_array                  0x09
#define amf3_object                 0x0a
#define amf3_xml                    0x0c
#define amf3_byte_array             0x0d
#define amf3_invalid                0xff

#define amf_new_bool(b)         amf_new(amf_boolean,(uint32_t)b)
#define amf_new_string(s,len)   amf_new(amf_string,(const char*)s,(int32_t)len)
#define amf_new_number(n)       amf_new(amf_number,(double)n)
#define amf_new_date(n,tz)      amf_new(amf_date,(double)n,(int32_t)tz)
#define amf_new_object()        amf_new(amf_object)
#define amf_new_ecma_array()    amf_new(amf_ecma_array)
#define amf_new_null()          amf_new(amf_null)

/*memory*/
typedef void* (*amf_malloc)(size_t size,void *u);
typedef void  (*amf_free)(void *p,void *u);

/*data types*/
typedef void *   amf_data_t;

char*   amf_get_string(amf_data_t *data);
uint8_t amf_get_bool(amf_data_t *data);
double  amf_get_number(amf_data_t *data);
double  amf_get_date(amf_data_t *data);

int amf_init(amf_malloc alloc,amf_free free,void *u);

amf_data_t* amf_new(uint8_t type,...);

uint8_t amf_data_type(amf_data_t * data);

void amf_free_data(amf_data_t * data);

int amf_put_prop(amf_data_t *obj,const char* name,const amf_data_t *data);
amf_data_t *amf_get_prop(amf_data_t *obj,const char* name);

/*return a amf object null for invalid*/
amf_data_t * amf_decode(const char *buf,int *len);

/*return remain length of buf*/
int amf_encode(amf_data_t *data,char *buf,int maxlen);

#ifdef HAVE_DEBUG
int amf_dump_data(amf_data_t * data);
#endif

#endif
