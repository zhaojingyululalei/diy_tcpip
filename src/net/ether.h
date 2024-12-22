#ifndef __ETHER_H
#define __ETHER_H

#include "types.h"
#include "net_cfg.h"
#define MTU_MAX_SIZE    1500
#define MTU_MIN_SIZE    46
#pragma pack(1)
typedef struct _ether_header_t
{
    uint8_t dest[MAC_ADDR_ARR_LEN];
    uint8_t src[MAC_ADDR_ARR_LEN];
    uint16_t protocal;
}ether_header_t;

typedef struct _ether_pkg_t
{
    ether_header_t head;
    uint8_t data[MTU_MAX_SIZE];
}ether_pkg_t;

#pragma pack()
#include "protocal.h"
#include "package.h"
#include "netif.h"
void ether_init(void);
int ether_raw_out(netif_t* netif,protocal_type_t type,const uint8_t* mac_dest,pkg_t* pkg);
#endif
