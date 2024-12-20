#ifndef __NET_DRIVE_H
#define __NET_DRIVE_H

#include <pcap/pcap.h>
#include "ipaddr.h"
typedef struct _netif_card_info_t
{
    int id;
    int used; //这块网卡是否被使用，还是空闲
    int avail;
    char ipv4[16];
    uint8_t mac[MAC_ADDR_ARR_LEN];
}netif_card_info_t;
#define PCAP_NETIF_DRIVE_ARR_MAX    10
typedef struct _net_drive_info_t
{
    netif_card_info_t pcap_netif_drive_arr[PCAP_NETIF_DRIVE_ARR_MAX];
    int num;
}net_drive_info_t;

//驱动初始化，网卡检测
void pcap_drive_init(void);
//获取一张未使用的网卡
netif_card_info_t* get_one_net_card(void);
netif_card_info_t* get_one_specified_card(int idx);
void put_one_net_card(int card_idx);



/*虚拟机新添加了一个网卡,only-host模式， 在netplan中自己进行了配置，命名为ens37*/
int pcap_find_device(const char *ip, char *name_buf);
int pcap_show_list(void) ;

pcap_t * pcap_device_open(const char* ip, const uint8_t* mac_addr);
int pcap_recv_pkg(pcap_t * handler,  const uint8_t **pkg_data);
int pcap_send_pkg(pcap_t* handler,const uint8_t* buffer,int size);

#endif