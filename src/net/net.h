#ifndef __NET_H
#define __NET_H

#include "threadpool.h"
extern threadpool_t netthread_pool;
void net_system_init(void);
void net_system_start(void);


#endif
