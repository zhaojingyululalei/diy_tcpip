#ifndef __EXMSG_H
#define __EXMSG_H
#include "net_err.h"
#define MSGQ_TBL_MAX_LIMIT   512  //消息队列那张桌子有多大
#define MEMPOLL_CTL_MAX_LIMIT  1024 //内存池管理的消息数量

typedef struct _exmsg_data_t{
    int testid;
}exmsg_data_t;

extern int testid;

net_err_t exmsg_init(void);
net_err_t exmsg_netif_to_msgq(void);
exmsg_data_t* exmsg_getmsg_from_msgq(void);

#endif
