#ifndef __NET_ERR_H
#define __NET_ERR_H
#include "debug.h"
#include <stdbool.h>
typedef enum _net_err_t
{
    NET_ERR_OK=0,
    NET_ERR_SYS=-1,
    NET_ERR_MEM=-2,
    NET_ERR_TIMEOUT = -3
}net_err_t;

#define CHECK_NET_ERR(result) \
    do { \
        if ((result) != NET_ERR_OK) { \
            dbg_error("ret value not ok\n");\
            return (result); \
        } \
    } while(0)

#define CHECK_NET_NULL(result) \
    do { \
        if ((result) == NULL) { \
            dbg_error("ret value is none\n");\
            return NET_ERR_MEM; \
        } \
    } while(0)


#endif
