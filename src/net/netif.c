#include "netif.h"
#include "mmpool.h"
#include "debug.h"
#include "net_drive.h"

int netif_set_ipaddr(netif_t *netif, const ipaddr_t *ipaddr)
{
    if (!netif || !ipaddr)
    {
        return -1; // 参数无效
    }
    netif->info.ipaddr = *ipaddr; // 赋值 IP 地址
    return 0;                     // 成功
}
int netif_set_gateway(netif_t *netif, const ipaddr_t *gateway)
{
    if (!netif || !gateway)
    {
        return -1; // 参数无效
    }
    netif->info.gateway = *gateway; // 赋值网关地址
    return 0;                       // 成功
}
int netif_set_mask(netif_t *netif, const ipaddr_t *mask)
{
    if (!netif || !mask)
    {
        return -1; // 参数无效
    }
    netif->info.mask = *mask; // 赋值子网掩码
    return 0;                 // 成功
}
int netif_set_mtu(netif_t *netif, int mtu)
{
    if (!netif || mtu <= 0)
    {
        return -1; // 参数无效
    }
    netif->mtu = mtu; // 设置 MTU
    return 0;         // 成功
}
int netif_set_name(netif_t *netif, const char *name)
{
    if (!netif || !name)
    {
        return -1; // 参数无效
    }
    size_t len = strlen(name);
    if (len >= NETIF_NAME_STR_MAX_LEN)
    {
        return -1; // 名称过长
    }
    strncpy(netif->info.name, name, NETIF_NAME_STR_MAX_LEN - 1); // 拷贝名称
    netif->info.name[NETIF_NAME_STR_MAX_LEN - 1] = '\0';         // 确保以 '\0' 结尾
    return 0;                                                    // 成功
}

int netif_set_mac(netif_t *netif, const uint8_t *mac)
{
    if (!netif || !mac)
    {
        return -1;
    }
    memcpy(netif->macaddr, mac, 6);
    return 0;
}
/**/
static mempool_t netif_pool;
static netif_t netifs[NETIF_MAX_NR * (sizeof(netif_t) + sizeof(list_node_t))];
static list_t netif_list;
static lock_t netif_list_locker;
static link_layer_t *link_layers[NETIF_TYPE_SIZE];
int netif_register_link_layer(link_layer_t *layer)
{
    netif_type_t type = layer->type;
    if (type >= NETIF_TYPE_SIZE || type <= 0)
    {
        dbg_error("register link layer fail\r\n");
        return -1;
    }
    link_layers[type] = layer;
    return 0;
}
void print_netif_list(void)
{
    list_node_t *current_node = list_first(&netif_list); // 获取链表的第一个节点

    dbg_info("Netif List:\n");
    dbg_info("---------------------------------------------------------------------------------\n");
    dbg_info("| ID   | Name          | Type       | IP           | MAC Address         |\n");
    dbg_info("|      |               |            | Gateway      |                    |\n");
    dbg_info("|      |               |            | Mask         |                    |\n");
    dbg_info("---------------------------------------------------------------------------------\n");

    while (current_node)
    {
        // 使用 list_node_parent 宏获取 netif_t 指针
        netif_t *netif = list_node_parent(current_node, netif_t, node);

        // 类型转换为字符串
        const char *type_str = "Unknown";
        switch (netif->info.type)
        {
        case NETIF_TYPE_NONE:
            type_str = "None";
            break;
        case NETIF_TYPE_LOOP:
            type_str = "Loopback";
            break;
        case NETIF_TYPE_ETH:
            type_str = "Ethernet";
            break;
        }

        // 转换 IP 地址、网关和掩码为字符串
        char ip_str[16], gateway_str[16], mask_str[16];
        ipaddr_n2s(&netif->info.ipaddr, ip_str, sizeof(ip_str));
        ipaddr_n2s(&netif->info.gateway, gateway_str, sizeof(gateway_str));
        ipaddr_n2s(&netif->info.mask, mask_str, sizeof(mask_str));

        // 格式化 MAC 地址为字符串
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 netif->macaddr[0], netif->macaddr[1], netif->macaddr[2],
                 netif->macaddr[3], netif->macaddr[4], netif->macaddr[5]);

        // 打印信息
        dbg_info("| %-4d | %-13s | %-10s | %-11s | %-17s |\n", netif->id, netif->info.name, type_str, ip_str, mac_str);
        dbg_info("|      |               |            | %-11s |                    |\n", gateway_str);
        dbg_info("|      |               |            | %-11s |                    |\n", mask_str);
        dbg_info("---------------------------------------------------------------------------------\n");

        // 获取下一个节点
        current_node = list_node_next(current_node);
    }
}
netif_t *is_mac_host(uint8_t *mac)
{
    list_t *list = &netif_list;
    list_node_t *cur_node = list->first;
    while (cur_node)
    {
        netif_t *cur_netif = list_node_parent(cur_node, netif_t, node);
        if (memcmp(mac, cur_netif->macaddr, MAC_ADDR_ARR_LEN) == 0)
        {
            return cur_netif;
        }
        cur_node = cur_node->next;
    }
    return NULL;
}

netif_t *is_ip_host(ipaddr_t *ip)
{
    list_t *list = &netif_list;
    list_node_t *cur_node = list->first;
    while (cur_node)
    {
        netif_t *cur_netif = list_node_parent(cur_node, netif_t, node);
        if (cur_netif->info.ipaddr.q_addr == ip->q_addr)
        {
            return cur_netif;
        }
        cur_node = cur_node->next;
    }
    return NULL;
}
void netif_init(void)
{

    mempool_init(&netif_pool, netifs, NETIF_MAX_NR, sizeof(netif_t));
    list_init(&netif_list);
    lock_init(&netif_list_locker);
}
void netif_destory(void)
{
    mempool_destroy(&netif_pool);
    lock_destory(&netif_list_locker);
    list_destory(&netif_list);
}
#include "phnetif.h"
/*注册物理网卡*/
netif_t *netif_register(const netif_info_t *info, const netif_ops_t *ops, void *ex_data)
{
    netif_t *netif = (netif_t *)mempool_alloc_blk(&netif_pool, -1);
    netif_card_info_t *card = ((phnetif_drive_data_t *)ex_data)->card_info;
    if (!netif)
    {
        dbg_warning("netif_pool out of memory\r\n");
        return NULL;
    }
    netif->ex_data = ex_data;
    netif->ops = ops;
    // 深拷贝info
    memcpy(&netif->info, info, sizeof(netif_info_t));
    free(info);
    netif->id = card->id;
    netif->mtu = 1500;
    for (int i = 0; i < 6; i++)
    {
        netif->macaddr[i] = card->mac[i];
    }

    return netif;
}
netif_t *netif_virtual_register(const netif_info_t *info, const netif_ops_t *ops, void *ex_data)
{
    netif_t *netif = (netif_t *)mempool_alloc_blk(&netif_pool, -1);
    memset(netif, 0, sizeof(netif));
    if (!netif)
    {
        dbg_warning("netif_pool out of memory\r\n");
        return NULL;
    }
    netif->ex_data = ex_data;
    netif->ops = ops;
    // 深拷贝info
    memcpy(&netif->info, info, sizeof(netif_info_t));
    free(info);
    netif->id = PCAP_NETIF_DRIVE_ARR_MAX + list_count(&netif_list);
    netif->mtu = 1500;

    return netif;
}
int netif_free(netif_t *netif)
{
    int ret;
    // 如果是物理网卡
    if (netif->id < PCAP_NETIF_DRIVE_ARR_MAX)
    {
        put_one_net_card(netif->id);
    }
    msgQ_destory(&netif->in_q);
    msgQ_destory(&netif->out_q);
    memset(netif, 0, sizeof(netif_t));
    ret = mempool_free_blk(&netif_pool, netif);
    if (ret < 0)
    {
        dbg_error("netif free fail\r\n");
        return ret;
    }
    return 0;
}

int netif_add(netif_t *netif)
{
    lock(&netif_list_locker);
    list_insert_last(&netif_list, &netif->node);
    unlock(&netif_list_locker);
    return 0;
}

netif_t *netif_remove(netif_t *netif)
{
    lock(&netif_list_locker);
    list_t *list = &netif_list;
    list_node_t *cur = list->first;
    while (cur)
    {
        if (cur == &netif->node)
        {
            list_remove(list, cur);
            unlock(&netif_list_locker);
            return netif;
        }
        cur = cur->next;
    }
    unlock(&netif_list_locker);
    return NULL;
}
netif_t *netif_first()
{
    list_t *list = &netif_list;
    list_node_t *fnode = list->first;
    netif_t *ret = list_node_parent(fnode, netif_t, node);
    return ret;
}
netif_t *netif_next(netif_t *netif)
{
    if (!netif)
    {
        dbg_error("netif is NULL,not have next\r\n");
        return NULL;
    }
    list_node_t *next = netif->node.next;
    if (!next)
    {
        return NULL;
    }
    else
    {
        return list_node_parent(next, netif_t, node);
    }
}

int netif_open(netif_t *netif)
{
    if (netif->state != NETIF_CLOSE)
    {
        dbg_error("netif state is not right,can not open\r\n");
        return -3;
    }
    netif->state = NETIF_OPEN;
    int ret;
    msgQ_init(&netif->in_q, netif->in_q_buf, NETIF_INQ_BUF_MAX);
    msgQ_init(&netif->out_q, netif->out_q_buf, NETIF_OUTQ_BUF_MAX);
    lock_init(&netif->locker);
    // 将网卡加入链表
    ret = netif_add(netif);
    if (ret < 0)
    {
        msgQ_destory(&netif->in_q);
        msgQ_destory(&netif->out_q);
        dbg_error("netif list insert fail\r\n");
        return ret;
    }

    ret = netif->ops->open(netif, netif->ex_data); // 调用驱动
    // 注册数据链路层操作
    netif->link_ops = link_layers[netif->info.type];
    if (ret < 0)
    {
        return ret;
    }
    return 0;
}
#include "net.h"
int netif_activate(netif_t *netif)
{
    if (!netif)
    {
        dbg_error("empty netif,can not activate\r\n");
        return -1;
    }
    if (!(netif->state == NETIF_OPEN || netif->state == NETIF_DIE))
    {
        dbg_error("netif state is wrong,can not activate\r\n");
        return -2;
    }
    netif->state = NETIF_ACTIVe;

    //
    if (!netif->link_ops)
    {
        dbg_error("link_ops nerver register\r\n");
        return -1;
    }
    // 激活收发线程开始工作
    lock(&netif->locker);
    netif->send_flag = 1;
    netif->recv_flag = 1;
    unlock(&netif->locker);
    thread_t *send = thread_create(&netthread_pool, netif_send, (void *)netif);
    thread_t *recv = thread_create(&netthread_pool, netif_receive, (void *)netif);
    lock(&netif->locker);
    netif->tsend = send;
    netif->trecv = recv;
    unlock(&netif->locker);

    // 链路层打开

    netif->link_ops->open(netif);

    return 0;
}

int netif_shutdown(netif_t *netif)
{
    if (!netif)
    {
        dbg_error("empty netif,can not activate\r\n");
        return -1;
    }
    if (netif->state != NETIF_ACTIVe)
    {
        dbg_error("netif state is wrong,can not shutdown\r\n");
        return -2;
    }
    int ret;
    netif->state = NETIF_DIE;
    if (netif->tsend)
    {
        lock(&netif->locker);
        netif->send_flag = 0;
        unlock(&netif->locker);
        ret = thread_join(&netthread_pool, netif->tsend, NULL);
        if (ret < 0)
        {
            dbg_error("send thread join fail\r\n");
        }
        else
        {
            lock(&netif->locker);
            netif->tsend = NULL;
            unlock(&netif->locker);
        }
    }
    if (netif->trecv)
    {
        lock(&netif->locker);
        netif->recv_flag = 0;
        unlock(&netif->locker);
        ret = thread_join(&netthread_pool, netif->trecv, NULL);
        if (ret < 0)
        {
            dbg_error("recv thread join fail\r\n");
        }
        else
        {
            lock(&netif->locker);
            netif->trecv = NULL;
            unlock(&netif->locker);
        }
    }

    while (!msgQ_is_empty(&netif->in_q))
    {
        pkg_t *pkg = (pkg_t *)msgQ_dequeue(&netif->in_q, -1);
        ret = package_collect(pkg);
        if (ret < 0)
        {
            dbg_warning("collect pkg fail\r\n");
        }
    }

    while (!msgQ_is_empty(&netif->out_q))
    {
        pkg_t *pkg = (pkg_t *)msgQ_dequeue(&netif->out_q, -1);
        package_collect(pkg);
        if (ret < 0)
        {
            dbg_warning("collect pkg fail\r\n");
        }
    }

    //
    if (netif->link_ops)
    {
        netif->link_ops->close(netif);
    }
    return 0;
}

int netif_close(netif_t *netif)
{
    int ret;
    if (netif->state != NETIF_DIE)
    {
        dbg_error("netif state is not right,can not close\r\n");
        return -3;
    }
    netif_t *rif = netif_remove(netif);
    if (!rif)
    {
        dbg_error("netif remove from list fail\r\n");
        return -2;
    }
    msgQ_destory(&netif->in_q);
    msgQ_destory(&netif->out_q);

    netif->state = NETIF_CLOSE;
    return 0;
}

int netif_out(netif_t *netif, ipaddr_t *ip, pkg_t *pkg)
{
    int ret;
    if (netif->link_ops)
    {
        ret = netif->link_ops->out(netif, ip, pkg);
        if (ret < 0)
        {
            dbg_warning("netif link out a pkg fail\r\n");
            package_collect(pkg);
            return ret;
        }
    }
    else
    {
        ret = netif_putpkg(&netif->out_q, pkg, -1);
        if (ret < 0)
        {
            dbg_warning("netif out a pkg fail\r\n");
            package_collect(pkg);
            return ret;
        }
    }
    return 0;
}

/*取包时阻塞等*/
pkg_t *netif_getpkg(msgQ_t *queue, int ms)
{
    return msgQ_dequeue(queue, ms);
}
/*放包时不用等，大不了丢包*/
int netif_putpkg(msgQ_t *queue, pkg_t *pkg, int ms)
{
    return msgQ_enqueue(queue, pkg, ms);
}
/**
 * 从outq里取出一个数据包，然后通过网卡发出去
 */
int netif_send_simulate(netif_t *netif)
{
    int ret;
    if (!netif)
    {
        return -1;
    }
    pkg_t *pkg = netif_getpkg(&netif->out_q, 0); // 阻塞等，队列里没东西就等
    if (!pkg)
    {
        return 0;
    }
    uint8_t *buf = malloc(pkg->total);
    memset(buf, 0, pkg->total);
    package_lseek(pkg, 0);
    package_read(pkg, buf, pkg->total);
    ret = netif->ops->send(netif, buf, pkg->total); // 发送驱动
    // 发送成功回收资源
    package_collect(pkg);
    free(buf);
    return ret;
}
/**
 * 从网卡接受一个数据包，放入inq
 */
#include "networker.h"
int netif_receive_simulate(netif_t *netif)
{
    if (!netif)
    {
        return -1;
    }
    int ret;
    uint8_t *buf = NULL;
    ret = netif->ops->receive(netif, &buf, PKG_DATA_BLK_SIZE); // 网卡驱动
    if (ret <= 0)
    {
        free(buf);
        return -1;
    }
    pkg_t *pkg = package_create(buf, ret);
    if (!pkg)
    {
        dbg_error("create a pkg fail\r\n");
        return -1;
    }

    ret = netif_putpkg(&netif->in_q, pkg, -1);
    if (ret == 0)
    {
        post(&mover_sem); // 确实放进去一个pkg，唤醒mover线程搬运
    }
    free(buf);
    return ret;
}

DEFINE_THREAD_FUNC(netif_send)
{
    netif_t *netif = (netif_t *)arg;
    lock(&netif->locker);
    int flag = netif->send_flag;
    unlock(&netif->locker);
    while (flag)
    {

        netif_send_simulate(netif);
        lock(&netif->locker);
        flag = netif->send_flag;
        unlock(&netif->locker);
    }
    dbg_info("++++++++++++++++++++++++++++netif send killed++++++++++++++++++++\r\n");
    return NULL;
}

DEFINE_THREAD_FUNC(netif_receive)
{
    netif_t *netif = (netif_t *)arg;
    lock(&netif->locker);
    int flag = netif->recv_flag;
    unlock(&netif->locker);
    while (flag)
    {
        netif_receive_simulate(netif);
        lock(&netif->locker);
        flag = netif->recv_flag;
        unlock(&netif->locker);
    }
    dbg_info("++++++++++++++++++++++++++++++netif recv killed+++++++++++++++++++++\r\n");
    return NULL;
}