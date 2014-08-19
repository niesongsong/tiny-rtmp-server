
/*
 * CopyLeft (C) nie950@gmail.com
 */

/*HAVE_NDEBUG*/
#define HAVE_DEBUG   1

#ifndef MEM_ALIGNMENT
#define MEM_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define mem_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define mem_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#ifdef _WIN32
#include <rtmp_win32_config.h>
#else
#include <rtmp_linux_config.h> 
#endif


#include <openssl/hmac.h>
#include <errno.h>

#include "rtmp_alloc.h"
