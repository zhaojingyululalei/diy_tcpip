#ifndef __NETIF_H
#define __NETIF_H
#include "net_err.h"
#include "net_cfg.h"
#include "ipaddr.h"
#include "list.h"
#include "msg_queue.h"
/**
 * @brief 网络接口类型
 */
typedef enum _netif_type_t {
    NETIF_TYPE_NONE = 0,                // 无类型网络接口
    NETIF_TYPE_ETHER,                   // 以太网
    NETIF_TYPE_LOOP,                    // 回环接口
}netif_type_t;

/**
 * @brief 硬件地址
 */
typedef struct _netif_hwaddr_t {                        // 地址长度
    uint8_t addr[NETIF_HWADDR_SIZE];        // 地址空间
}netif_hwaddr_t;


typedef struct _netif_t
{
    char name[NETIF_NAME_SIZE];
    netif_hwaddr_t hwaddr;
    ipaddr_t ipaddr;
    ipaddr_t netmask;
    ipaddr_t gateway;

    enum{
        NETIF_CLOSED,
        NETIF_OPEN,
        NETIF_ACTIVE,
    }state;

    netif_type_t type;
    int mtu;

    list_node_t node;
    msgQ_t in_q;
    void* in_buf[NETIF_INQ_BUF_SIZE];
    msgQ_t out_q;
    void* out_buf[NETIF_OUTQ_BUF_SIZE];
    
}netif_t;


//父类独有的方法
net_err_t netif_takein_list(netif_t* netif);
net_err_t netif_takeout_list(netif_t* netif);
netif_t* netif_get_first(void);
netif_t* netif_get_next(netif_t* cur);


net_err_t netif_init(void);
void netif_show_info(netif_t *netif);
void netif_show_list_info(void) ;


//被子类重写的方法
netif_t* netif_open(const char* name,void* data);
void netif_close(netif_t* netif);
net_err_t netif_active(netif_t* netif);
net_err_t netif_deactive(netif_t* netif);
net_err_t netif_transsimit(netif_t* netif);
net_err_t netif_set_ipaddr(netif_t* netif,ipaddr_t* ip,ipaddr_t* mask,ipaddr_t* gateway);
net_err_t netif_set_hwaddr(netif_t*netif,netif_hwaddr_t* hwaddr);
#endif

