#ifndef __NETIF_H
#define __NETIF_H

#include "list.h"
#include "types.h"
#include "ipaddr.h"
#include "msgQ.h"
#include "net_drive.h"
#include "net_cfg.h"
#include "package.h"

#define NETIF_NAME_STR_MAX_LEN  128

typedef struct _netif_info_t
{
    enum{
        NETIF_TYPE_NONE,
        NETIF_TYPE_LOOP,
        NETIF_TYPE_ETH,
    }type;
    char name[NETIF_NAME_STR_MAX_LEN];
    ipaddr_t ipaddr;
    ipaddr_t mask;
    ipaddr_t gateway;
    
}netif_info_t;

/**
 * 1.不同网卡寄存器操作不同
 * 2.网卡类型不同，虚拟，物理，loop等操作都不同
 * */
struct _netif_t;
typedef struct _netif_ops_t
{
    int (*open) (struct _netif_t* netif,void* ex_data); //不同驱动可能有些不同参数
    int (*close) (struct _netif_t* netif);
    int (*send) (struct _netif_t* netif,const uint8_t* buf,int len);
    int (*receive) (struct _netif_t* netif,uint8_t* buf,int len);
}netif_ops_t;
typedef struct _netif_t
{
    
    netif_info_t info;
    enum{
        NETIF_CLOSE,
        NETIF_OPEN,
        NETIF_ACTIVe,
        NETIF_DIE
    }state;

    int id;
    
    uint8_t macaddr[6];
    int mtu;
    list_node_t node;

    msgQ_t in_q;
    pkg_t* in_q_buf[NETIF_INQ_BUF_MAX];
    msgQ_t out_q;
    pkg_t* out_q_buf[NETIF_OUTQ_BUF_MAX];

    netif_ops_t* ops;
    void* ex_data;

}netif_t;


uint8_t* get_mac_addr(const char* name);

void netif_init(void);
void netif_destory(void);

/********************* */
//网卡注册
netif_t* netif_register(const netif_info_t* info,const netif_ops_t* ops,const void* ex_data);
//虚拟网卡注册
netif_t* netif_virtual_register(const netif_info_t* info,const netif_ops_t* ops,const void* ex_data);
//网卡释放
int netif_free(netif_t* netif);

int netif_open(netif_t* netif,void* ex_data);
int netif_activate(netif_t* netif);
int netif_shutdown(netif_t* netif);
/***************** */


/******************* */
//网卡链表操作,涉及到链表操作，都得上锁
int netif_add(netif_t* netif);
netif_t* netif_remove(netif_t* netif);
void print_netif_list(void);
/******************** */


/*配置函数*/
int netif_set_ipaddr(netif_t* netif, const ipaddr_t* ipaddr);
int netif_set_gateway(netif_t* netif, const ipaddr_t* gateway);
int netif_set_mask(netif_t* netif, const ipaddr_t* mask);
int netif_set_mtu(netif_t* netif, int mtu);
int netif_set_name(netif_t* netif, const char* name);
int netif_set_mac(netif_t* netif,const uint8_t* mac);


#endif