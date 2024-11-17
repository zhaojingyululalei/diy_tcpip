#ifndef __LOOP_H
#define __LOOP_H
#include "netif.h"

typedef struct _lo_netif_t{
    netif_t* netif; //继承
}lo_netif_t;


net_err_t lo_open(lo_netif_t* lo,const char* name,void* data);

net_err_t lo_close(lo_netif_t* lo);

net_err_t lo_transsimit(lo_netif_t* lo);

net_err_t lo_active(lo_netif_t* lo);

net_err_t lo_deactive(lo_netif_t* lo);
net_err_t lo_set_ipaddr(lo_netif_t* lo,ipaddr_t* ip,ipaddr_t* mask,ipaddr_t* gateway);
net_err_t lo_set_hwaddr(lo_netif_t* lo,netif_hwaddr_t* hwaddr);


#endif