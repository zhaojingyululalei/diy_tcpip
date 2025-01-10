#include "ether.h"
#include "netif.h"
#include "protocal.h"
#include "networker.h"
#include "arp.h"
static int(ether_open)(struct _netif_t *netif)
{
    int ret;
    // 发送一个无回报arp包，
    // 1.告诉别人自己的mac，便于让别人更新arp缓存
    // 2.探测ip地址是否冲突
    ret = arp_send_no_reply(netif);
    if (ret < 0)
    {
        return ret;
    }
    return 0;
}
static void(ether_close)(struct _netif_t *netif)
{
    return;
}
static int ether_pkg_is_ok(netif_t *netif, ether_header_t *header, int pkg_len)
{
    if (pkg_len > sizeof(ether_header_t) + MTU_MAX_SIZE)
    {
        dbg_warning("ether pkg len over size\r\n");
        return -1;
    }

    if (pkg_len < sizeof(ether_header_t))
    {
        dbg_warning("ether pkg len so small\r\n");
        return -2;
    }
    if (memcmp(netif->macaddr, header->dest, MAC_ADDR_ARR_LEN) != 0 && memcmp(get_mac_broadcast(), header->dest, MAC_ADDR_ARR_LEN) != 0)
    {
        dbg_error("ether recv a pkg,dest mac addr wrong\r\n");
        return -3;
    }
    return 0;
}

static void ether_dbg_print_pkg(pkg_t *pkg)
{
#ifdef DBG_EHTER_PRINT
    dbg_info("***************ether link layer handling a pkg***********\r\n");
    uint8_t data[MTU_MAX_SIZE];
    ether_header_t *header = package_data(pkg, sizeof(ether_header_t), 0);
    char destbuf[18] = {0};
    char srcbuf[18] = {0};
    mac_n2s(header->dest, destbuf);
    mac_n2s(header->src, srcbuf);
    uint16_t pro_type = 0;
    n2h(&header->protocal, 2, &pro_type); // type占2字节
    dbg_info("dest_mac:%s\r\n", destbuf);
    dbg_info("src_mac:%s\r\n", srcbuf);
    dbg_info("protocal:%04x\r\n", pro_type);
    dbg_info("data:\r\n");
    int data_len = pkg->total - sizeof(ether_header_t);
    package_read_pos(pkg, data, data_len, sizeof(ether_header_t));
    for (int i = 0; i < data_len; ++i)
    {

        dbg_info("%02x ", data[i]);
    }
    dbg_info("\r\n");
    dbg_info("*********************************************\r\n");
#endif
}
#include "ipv4.h"
#include "arp.h"
static int update_arp_from_ipv4(netif_t* netif,pkg_t* pkg)
{
    if(!netif||!pkg)
    {
        return -1;
    }

    package_integrate_header(pkg,sizeof(ipv4_header_t)+sizeof(ether_header_t));
    ether_header_t* ether_head = package_data(pkg,sizeof(ether_header_t),0);
    ipv4_header_t* ipv4_head = package_data(pkg,sizeof(ether_header_t)+sizeof(ipv4_header_t),sizeof(ether_header_t));
    ipv4_head_parse_t ipv4_parse_head;
    parse_ipv4_header(ipv4_head,&ipv4_parse_head);
    ipv4_show_pkg(&ipv4_parse_head);
    if(!ipv4_pkg_is_ok(&ipv4_parse_head,ipv4_head))
    {
        dbg_warning("ipv4 format not right\r\n");
        return -2;
    }
    ipaddr_t ip = {
        .type = IPADDR_V4,
        .q_addr = ipv4_parse_head.src_ip
    };
    //如果源ip不和自己在同一网段，不更新arp
    if(ipaddr_get_net(&ip,&netif->info.mask)!=ipaddr_get_net(&netif->info.ipaddr,&netif->info.mask))
    {
        return -3;
    }
    arp_entry_t *entry = arp_cache_find(&arp_cache_table,&ip);
    if(entry)
    {
        return 0;
    }

    entry = arp_cache_alloc_entry(&arp_cache_table,0);
    entry->ip.q_addr = ipv4_parse_head.src_ip;
    memcpy(entry->mac,ether_head->src,MAC_ADDR_ARR_LEN);
    entry->tmo = ARP_ENTRY_TMO_STABLE;
    entry->retry = ARP_ENTRY_RETRY;
    entry->netif = netif;
    arp_cache_insert_entry(&arp_cache_table,entry);
    arp_show_cache_list();

    return 0;
}
static int(ether_in)(struct _netif_t *netif, pkg_t *package)
{
    int ret;

    ether_header_t *header = package_data(package, sizeof(ether_header_t), 0);
    if (ether_pkg_is_ok(netif, header, package->total) < 0)
    {
        dbg_warning("ether pkg problem,can not handle\r\n");
        return -1;
    }

    ether_dbg_print_pkg(package);
    uint16_t protocal = 0;
    _ntohs(header->protocal, &protocal);

    switch (protocal)
    {
    case PROTOCAL_TYPE_ARP:
        // 去掉以太网头
        ret = package_shrank_front(package, sizeof(ether_header_t));
        if (ret < 0)
        {
            dbg_error("package_shrank_fail\r\n");
            return ret;
        }
        ret = arp_in(netif, package);
        if (ret < 0)
        {
            dbg_warning("arp pkg handle \r\n");
            return ret;
        }
        break;
    case PROTOCAL_TYPE_IPV4:
        // 如果收到了一个ipv4数据包，有ip有mac可以更新arp表
        update_arp_from_ipv4(netif,package);
        // 去掉以太网头
        ret = package_shrank_front(package, sizeof(ether_header_t));
        if (ret < 0)
        {
            dbg_error("package_shrank_fail\r\n");
            return ret;
        }
        ret = ipv4_in(netif, package);
        if (ret < 0)
        {
            dbg_warning("arp pkg handle \r\n");
            return ret;
        }
        break;
    default:
        dbg_warning("unkown protocal pkg\r\n");
        return -1;
    }
    return 0;
}

int ether_raw_out(netif_t *netif, protocal_type_t type, const uint8_t *mac_dest, pkg_t *pkg)
{
    int ret;
    int size = pkg->total;
    if (size < MTU_MIN_SIZE)
    {
        // 不足46字节，调整至46字节
        package_expand_last(pkg, MTU_MIN_SIZE - size);
        package_memset(pkg, size, 0, MTU_MIN_SIZE - size);
    }

    ret = package_add_headspace(pkg, sizeof(ether_header_t));
    if (ret < 0)
    {
        dbg_error("pkg add header fail\r\n");
        return ret;
    }
    // 填充以太网数据包头
    ether_header_t *head = package_data(pkg, sizeof(ether_header_t), 0);
    memcpy(head->src, netif->macaddr, MAC_ADDR_ARR_LEN);
    memcpy(head->dest, mac_dest, MAC_ADDR_ARR_LEN);
    h2n(&type, 2, &head->protocal);

    ether_dbg_print_pkg(pkg); // 打印包信息

    netif_t *target = is_mac_host(mac_dest); // 检测目标网卡是不是主机网卡
    if (target)
    {
        // 如果是主机上的网卡，并且还是激活状态
        if (target->state == NETIF_ACTIVe)
        {
            ret = netif_putpkg(&target->in_q, pkg, -1); // 直接把包放入对应的输入队列
            if (ret == 0)
            {
                post(&mover_sem); // 放入成功，唤醒搬运线程搬运
            }
        }
    }
    else
    {
        // 直接放到本网卡输出队列，等待发送
        netif_putpkg(&netif->out_q, pkg, -1);
    }
}
static int(ether_out)(struct _netif_t *netif, ipaddr_t *dest, pkg_t *package)
{
    int ret;
    netif_t *target = is_ip_host(dest);
    // 是本机ip直接将数据包放到对应网卡的接受缓存里
    if (target)
    {
        if (target->state == NETIF_ACTIVe)
        {
            ret = ether_raw_out(netif, PROTOCAL_TYPE_IPV4, target->macaddr, package);
            if (ret < 0)
            {
                return ret;
            }
        }
    }
    else
    {

        // 如果是外部ip,

        if (is_local_boradcast(netif, dest) || is_global_boradcast(dest))
        {
            // 如果是广播ip,直接发，不用查看arp缓存
            ret = ether_raw_out(netif, PROTOCAL_TYPE_IPV4, get_mac_broadcast(), package);
            if (ret < 0)
            {
                return ret;
            }
        }
        // 否则
        //  查找arp缓存表
        arp_entry_t *entry = arp_cache_find(&arp_cache_table, dest);
        if (entry)
        {
            uint8_t *mac_dest = entry->mac;
            if (!is_mac_empty(mac_dest))
            {
                ret = ether_raw_out(netif, PROTOCAL_TYPE_IPV4, mac_dest, package);
                if (ret < 0)
                {
                    return ret;
                }
            }
            else
            {
                // mac_dest为全0，还没解析出来
                // 把数据包放缓存里，等解析出mac地址，一起发出去
                if (list_count(&entry->pkg_list) < ARP_ENTRY_PKG_CACHE_MAX_SIZE)
                {
                    arp_entry_insert_pkg(entry, package);
                }
                else
                {
                    dbg_warning("too many pkg in entry pkg cache,drop one\r\n");
                }
            }
        }
        else
        {
            // mac_dest为null，根本没有这个ip的表项
            arp_entry_t *new_entry = arp_cache_alloc_entry(&arp_cache_table, 1);
            memset(new_entry, 0, sizeof(arp_entry_t));
            new_entry->ip.q_addr = dest->q_addr;
            new_entry->ip.type = dest->type;
            memset(new_entry->mac, 0, MAC_ADDR_ARR_LEN);
            new_entry->state = ARP_ENTRY_STATE_WAITING;
            new_entry->netif = netif;
            new_entry->tmo = ARP_ENTRY_TMO_STABLE;
            new_entry->retry = ARP_ENTRY_RETRY;
            // 分配表项，并加入arp cache table
            arp_cache_insert_entry(&arp_cache_table, new_entry);
            // 将数据包放入entry pkg list缓存
            arp_entry_insert_pkg(new_entry, package);
            arp_show_cache_list();
            // 发送arp请求包
            ret = arp_send_request(netif, dest);
            return ret;
        }
    }
    return 0;
}
static const link_layer_t ether_link_layer = {
    .type = NETIF_TYPE_ETH,
    .open = ether_open,
    .close = ether_close,
    .in = ether_in,
    .out = ether_out,
};
void ether_init(void)
{
    netif_register_link_layer(&ether_link_layer);
}
