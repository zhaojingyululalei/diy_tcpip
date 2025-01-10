#ifndef __NETIF_H
#define __NETIF_H

#include "list.h"
#include "types.h"
#include "ipaddr.h"
#include "msgQ.h"
#include "net_drive.h"
#include "net_cfg.h"
#include "package.h"

#define NETIF_NAME_STR_MAX_LEN 128
typedef enum _netif_type_t
{
    NETIF_TYPE_NONE,
    NETIF_TYPE_LOOP,
    NETIF_TYPE_ETH,

    NETIF_TYPE_SIZE
} netif_type_t;
typedef struct _netif_info_t
{
    netif_type_t type;
    char name[NETIF_NAME_STR_MAX_LEN];
    ipaddr_t ipaddr;
    ipaddr_t mask;
    ipaddr_t gateway;

} netif_info_t;

/**
 * 1.不同网卡寄存器操作不同
 * 2.网卡类型不同，虚拟，物理，loop等操作都不同
 * */
struct _netif_t;
typedef struct _netif_ops_t
{
    int (*open)(struct _netif_t *netif, void *ex_data); // 不同驱动可能有些不同参数
    int (*close)(struct _netif_t *netif);
    // len:要发送的长度  return 成功发送的数据长度
    int (*send)(struct _netif_t *netif, const uint8_t *buf, int len);
    // len:buf缓冲区的长度  return 接收到的数据长度
    int (*receive)(struct _netif_t *netif, uint8_t **buf);
} netif_ops_t;

/**
 *  链路层处理接口
 */
typedef struct _link_layer_t
{
    netif_type_t type;

    int (*open)(struct _netif_t *netif);
    void (*close)(struct _netif_t *netif);
    int (*in)(struct _netif_t *netif, pkg_t *package);
    int (*out)(struct _netif_t *netif, ipaddr_t *dest, pkg_t *package);
} link_layer_t;
typedef struct _netif_t
{

    netif_info_t info;
    enum
    {
        NETIF_CLOSE,
        NETIF_OPEN,
        NETIF_ACTIVe,
        NETIF_DIE
    } state;

    int id;

    uint8_t macaddr[MAC_ADDR_ARR_LEN];
    int mtu;
    list_node_t node;

    msgQ_t in_q;
    pkg_t *in_q_buf[NETIF_INQ_BUF_MAX];
    msgQ_t out_q;
    pkg_t *out_q_buf[NETIF_OUTQ_BUF_MAX];

    netif_ops_t *ops; // 驱动
    void *ex_data;    // 驱动参数
    link_layer_t* link_ops; //链路层处理

    int send_flag; // 杀死线程的标志,网卡停止工作的时候
    int recv_flag;
    thread_t *tsend;
    thread_t *trecv;

    lock_t locker;

} netif_t;

//uint8_t *get_mac_addr(const char *name);

void netif_init(void);
void netif_destory(void);

/********************* */
/*wifi 以太等数据链路层操作不同，这里做成回调函数形式*/
int netif_register_link_layer(link_layer_t* layer);

// 网卡注册
netif_t *netif_register(const netif_info_t *info, const netif_ops_t *ops, void *ex_data);
// 虚拟网卡注册
netif_t *netif_virtual_register(const netif_info_t *info, const netif_ops_t *ops, void *ex_data);
// 网卡释放
int netif_free(netif_t *netif);

int netif_open(netif_t *netif);
int netif_activate(netif_t *netif);
int netif_shutdown(netif_t *netif);
int netif_close(netif_t *netif);
int netif_out(netif_t *netif, ipaddr_t *ip, pkg_t *pkg);

pkg_t *netif_getpkg(msgQ_t *queue,int ms);
int netif_putpkg(msgQ_t *queue, pkg_t *pkg,int ms);

// 这里暂时用线程做，移植后，用中断代替
int netif_send_simulate(netif_t *netif);
int netif_receive_simulate(netif_t *netif);

DEFINE_THREAD_FUNC(netif_send);
DEFINE_THREAD_FUNC(netif_receive);
/***************** */

/******************* */
// 网卡链表操作,涉及到链表操作，都得上锁
int netif_add(netif_t *netif);
netif_t *netif_remove(netif_t *netif);
netif_t *netif_first();
netif_t *netif_next(netif_t *netif);
void print_netif_list(void);

netif_t* get_netif_accord_ip(ipaddr_t* ip);
netif_t* is_mac_host(uint8_t* mac);
netif_t* is_ip_host(ipaddr_t* ip);

/******************** */

/*配置函数*/
int netif_set_ipaddr(netif_t *netif, const ipaddr_t *ipaddr);
int netif_set_gateway(netif_t *netif, const ipaddr_t *gateway);
int netif_set_mask(netif_t *netif, const ipaddr_t *mask);
int netif_set_mtu(netif_t *netif, int mtu);
int netif_set_name(netif_t *netif, const char *name);
int netif_set_mac(netif_t *netif, const uint8_t *mac);


int is_local_boradcast(netif_t *netif,ipaddr_t * ip);
int is_global_boradcast(ipaddr_t* ip);
#endif
