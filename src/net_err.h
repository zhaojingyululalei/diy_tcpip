#ifndef __NET_ERR_H
#define __NET_ERR_H
#include "debug.h"
typedef enum _net_err_t
{
    NET_ERR_OK=0,
    NET_ERR_SYS=-1,
}net_err_t;

#define CHECK_NET_ERR(result) \
    do { \
        if ((result) != NET_ERR_OK) { \
            dbg_warning("ret value not ok\n");\
            return (result); \
        } \
    } while(0)



#endif
