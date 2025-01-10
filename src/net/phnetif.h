#ifndef __PHNETIF_H
#define __PHNETIF_H
#include "net_drive.h"
#include "netif.h"
/*物理网卡驱动信息*/
typedef struct _phnetif_drive_data_t
{
    netif_card_info_t * card_info;
    pcap_t* handler;
}phnetif_drive_data_t;
netif_t *phnetif_init(void);
int phnetif_open(netif_t *netif, void *ex_data);
int phnetif_close(netif_t *netif);
int phnetif_send(netif_t *netif, const uint8_t *buf, int len);
int phnetif_receive(netif_t *netif, const uint8_t **buf);

#endif