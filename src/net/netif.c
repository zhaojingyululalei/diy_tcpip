#include "include/netif.h"
#include "thread.h"
#include "include/exmsg.h"
#include "mempool.h"
#include <string.h>
#include "netif.h"
#include "package.h"

static uint8_t netif_buf[NETIF_MAX_CNT * (sizeof(list_node_t) + sizeof(netif_t))];
static mempool_t netif_pool;
static list_t netif_list; // 网络接口列表,所有open的网卡放在这里

static DEFINE_THREAD_FUNC(receiver)
{
    printf("receiver working......\n");
    while (1)
    {
        exmsg_netif_to_msgq();
        sleep(1);
    }

    return NULL;
}

static DEFINE_THREAD_FUNC(transimtor)
{
    printf("transimtor working......\n");
    while (1)
    {
        sleep(1);
    }

    return NULL;
}

// 初始化一块网卡资源
net_err_t netif_init(void)
{
    net_err_t ret;
    list_init(&netif_list);
    printf("netif init...\n");
    ret = mempool_init(&netif_pool, netif_buf, NETIF_MAX_CNT, sizeof(netif_t));
    if (ret != NET_ERR_OK)
        return NET_ERR_SYS;

    printf("netif init finish...\n");
    return NET_ERR_OK;
}

net_err_t netif_active(netif_t *netif)
{
    if (netif->state != NETIF_OPEN)
    {
        dbg_error("netif not open yet,can not active\n");
        return NET_ERR_SYS;
    }
    netif->state = NETIF_ACTIVE;
    return NET_ERR_OK;
}
net_err_t netif_deactive(netif_t *netif)
{
    if (netif->state != NETIF_ACTIVE)
    {
        dbg_error("netif not ACTIVED,can not deactive\n");
        return NET_ERR_SYS;
    }
    netif->state = NETIF_OPEN;

    // 未激活状态无法进行数据包收发功能
    // 把消息队列中所有的数据包都取出来并释放掉，
    while (!msgQ_empty(&netif->in_q))
    {
        pkg_t *package = msgQ_dequeue(&netif->in_q, -1);
        package_collect(package);
    }

    while (!msgQ_empty(&netif->out_q))
    {
        pkg_t *package = msgQ_dequeue(&netif->out_q, -1);
        package_collect(package);
    }

    return NET_ERR_OK;
}

netif_t *netif_open(const char *name, void *data)
{
    net_err_t ret;
    netif_t *netif = mempool_alloc_blk(&netif_pool, -1);
    netif_takein_list(netif);
    netif->state = NETIF_OPEN;
    strncpy(netif->name, name, strlen(name));
    netif->type = NETIF_TYPE_NONE;

    ret = msgQ_init(&netif->in_q, netif->in_buf, NETIF_INQ_BUF_SIZE);
    if (ret != NET_ERR_OK)
    {
        mempool_free_blk(&netif_pool, netif);
        return NULL;
    }

    ret = msgQ_init(&netif->out_q, netif->out_buf, NETIF_OUTQ_BUF_SIZE);
    if (ret != NET_ERR_OK)
    {
        mempool_free_blk(&netif_pool, netif);
        return NULL;
    }

    printf("netif init finish...\n");
    return netif;
}

void netif_close(netif_t *netif)
{
    if (netif->state == NETIF_ACTIVE)
    {
        dbg_error("netif current state is ACTIVE,please deactive fisrt\n");
        return NET_ERR_SYS;
    }

    netif_takeout_list(netif);
    memset(netif->name, 0, NETIF_NAME_SIZE);
    netif->state = NETIF_CLOSED;
    msgQ_destory(&netif->in_q);
    msgQ_destory(&netif->out_q);

    mempool_free_blk(&netif_pool, netif);
}

net_err_t netif_transsimit(netif_t *netif)
{
    return NET_ERR_OK;
}
net_err_t netif_set_ipaddr(netif_t *netif, ipaddr_t *ip, ipaddr_t *mask, ipaddr_t *gateway)
{
    if (ip)
    {

        memcpy(&netif->ipaddr, ip, sizeof(ipaddr_t));
    }
    if (mask)
    {

        memcpy(&netif->netmask, mask, sizeof(ipaddr_t));
    }
    if (gateway)
    {

        memcpy(&netif->gateway, gateway, sizeof(ipaddr_t));
    }

    return NET_ERR_OK;
}

net_err_t netif_set_hwaddr(netif_t *netif, netif_hwaddr_t *hwaddr)
{
    if (hwaddr)
    {

        memcpy(&netif->hwaddr, hwaddr, sizeof(netif_hwaddr_t));
    }
    return NET_ERR_OK;
}

net_err_t netif_takein_list(netif_t *netif)
{
    if (!netif)
    {
        return NET_ERR_SYS;
    }
    list_insert_last(&netif_list, &netif->node);
    return NET_ERR_OK;
}
net_err_t netif_takeout_list(netif_t *netif)
{
    if (!netif)
    {
        return NET_ERR_SYS;
    }
    list_remove(&netif_list, &netif->node);
    return NET_ERR_OK;
}

netif_t *netif_get_first(void)
{
    list_node_t *fnode = list_first(&netif_list);
    netif_t *first = list_node_parent(fnode, netif_t, node);
    return first;
}

netif_t *netif_get_next(netif_t *cur)
{
    if (!cur)
    {
        return NULL;
    }
    list_node_t *curnode = &cur->node;
    list_node_t *nextnode = list_node_next(curnode);
    if (nextnode == NULL)
    {
        return NULL;
    }
    netif_t *next = list_node_parent(nextnode, netif_t, node);
    return next;
}

void netif_show_info(netif_t *netif)
{
    if (netif == NULL)
    {
        printf("Invalid network interface\n");
        return;
    }

    // 打印网络接口基本信息
    printf("Network Interface: %s\n", netif->name);
    printf("  State: %s\n",
           netif->state == NETIF_CLOSED ? "Closed" : netif->state == NETIF_OPEN ? "Open"
                                                                                : "Active");
    printf("  Type: %s\n",
           netif->type == NETIF_TYPE_ETHER ? "Ethernet" : netif->type == NETIF_TYPE_LOOP ? "Loopback"
                                                                                         : "None");
    printf("  MTU: %d\n", netif->mtu);

    // 打印硬件地址
    printf("  Hardware Address: ");
    for (int i = 0; i < NETIF_HWADDR_SIZE; i++)
    {
        printf("%x%s", netif->hwaddr.addr[i], (i == NETIF_HWADDR_SIZE - 1) ? "" : ":");
    }
    printf("\n");

    // 打印IP地址
    printf("  IP Address: %d.%d.%d.%d\n",
           netif->ipaddr.a_addr[3], netif->ipaddr.a_addr[2],
           netif->ipaddr.a_addr[1], netif->ipaddr.a_addr[0]);

    // 打印子网掩码
    printf("  Netmask: %d.%d.%d.%d\n",
           netif->netmask.a_addr[3], netif->netmask.a_addr[2],
           netif->netmask.a_addr[1], netif->netmask.a_addr[0]);

    // 打印网关
    printf("  Gateway: %d.%d.%d.%d\n",
           netif->gateway.a_addr[3], netif->gateway.a_addr[2],
           netif->gateway.a_addr[1], netif->gateway.a_addr[0]);

    printf("\n");
}

void netif_show_list_info(void)
{
    printf("\n");
    printf("Network Interface List:\n");
    printf("=========================================\n");

    netif_t *cur = netif_get_first();
    while (cur)
    {
        netif_show_info(cur);
        cur = netif_get_next(cur);
    }

    printf("=========================================\n");
}
