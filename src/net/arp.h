#ifndef __ARP_H
#define __ARP_H
#include "types.h"
#include "ipaddr.h"
#include "list.h"
#include "netif.h"
#include "mmpool.h"
#define ARP_ENTRY_MAX_SIZE  64
typedef struct _arp_entry_t
{
    ipaddr_t ip;
    uint8_t mac[MAC_ARRAY_LEN];
    list_t pkg_list;//发包时，发现没有mac地址，先把包缓存到这里，然后去找mac地址
    netif_t* netif; //从哪个网卡发包
    enum{
        ARP_ENTRY_STATE_NONE,
        ARP_ENTRY_STATE_WAITING,//正在找mac地址
        ARP_ENTRY_STATE_RESOLVED,//找到了

    }state;

    list_node_t node;
}arp_entry_t;

typedef struct _arp_cache_table_t
{
    list_t entry_list;
    mempool_t entry_pool;//内存池，分配entry_t用的
    uint8_t entry_buff[ARP_ENTRY_MAX_SIZE*(sizeof(arp_entry_t)+sizeof(list_node_t))];
}arp_cache_table_t;

#define ARP_HARD_TYPE_ETHER 1
#define ARP_PROTOCAL_TYPE_IPV4  0x0800
#define ARP_OPCODE_REQUEST  1
#define ARP_OPCODE_REPLY    2
#pragma pack(1)
typedef struct _arp_pkg_t
{
    uint16_t hard_type;
    uint16_t protocal_type;
    uint8_t hard_len;
    uint8_t protocal_len;
    uint16_t opcode;
    uint8_t src_mac[MAC_ADDR_ARR_LEN];
    uint32_t src_ip;
    uint8_t dest_mac[MAC_ADDR_ARR_LEN];
    uint32_t dest_ip;
}arp_pkg_t;
#pragma pack(0)



void arp_init(void);
int arp_send_request(netif_t* netif, const ipaddr_t* dest_ip);
int arp_send_no_reply(netif_t* netif);
int arp_send_reply(netif_t* netif,pkg_t* pkg);
#endif
