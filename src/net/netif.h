#ifndef __NETIF_H
#define __NETIF_H

#include "list.h"
#include "types.h"
#include "ipaddr.h"
#include "msgQ.h"
#include "net_cfg.h"

#define NETIF_NAME_STR_MAX_LEN  128

typedef struct _netif_t
{
    enum{
        NETIF_TYPE_NONE,
        NETIF_TYPE_LOOP,
        NETIF_TYPE_ETH,
    }type;

    enum{
        NETIF_CLOSE,
        NETIF_OPEN,
        NETIF_ACTIVe,
    }state;

    int id;
    char name[NETIF_NAME_STR_MAX_LEN];
    ipaddr_t ipaddr;
    ipaddr_t mask;
    ipaddr_t gateway;
    uint8_t macaddr[6];

    int mtu;
    list_node_t node;

    msgQ_t in_q;
    void* in_q_buf[NETIF_INQ_BUF_MAX];
    msgQ_t out_q;
    void* out_q_buf[NETIF_OUTQ_BUF_MAX];

}netif_t;


uint8_t* get_mac_addr(const char* name);
#endif
