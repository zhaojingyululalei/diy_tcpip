#ifndef __LOOP_H
#define __LOOP_H
#include "netif.h"
netif_t* loop_init(void);
int loop_open(netif_t* netif,void* ex_data);
int loop_close(netif_t* netif);
int loop_send(netif_t* netif,const uint8_t* buf,int len);
int loop_receive(netif_t* netif,uint8_t* buf,int len);
#endif
