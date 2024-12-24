#include "arp.h"
#include "ether.h"
arp_cache_table_t arp_cache_table;

static int arp_show_cache_entry(arp_entry_t *entry)
{
#ifdef DBG_ARP_PRITN
    char ip_buf[20] = {0};
    char mac_buf[20] = {0};
    char *state;
    ipaddr_n2s(&entry->ip, ip_buf, 20);
    mac_n2s(entry->mac, mac_buf);
    switch (entry->state)
    {
    case ARP_ENTRY_STATE_NONE:
        state = "NONE";
        break;
    case ARP_ENTRY_STATE_WAITING:
        state = "waiting";
        break;
    case ARP_ENTRY_STATE_RESOLVED:
        state = "resolved";
        break;
    default:
        dbg_error("unkown state\r\n");
        return -1;
    }
    dbg_info("-----\r\n");
    dbg_info("cache entry ip:%s\r\n", ip_buf);
    dbg_info("cache entry mac:%s\r\n", mac_buf);
    dbg_info("cache entry state:%s\r\n", state);
    dbg_info("cache entry netif:%s\r\n", entry->netif->info.name);
    dbg_info("cache entry cache (%d) pkgs\r\n", entry->pkg_list.count);
    dbg_info("-----\r\n");
#endif
    return 0;
}
void arp_show_cache_list(void)
{
#ifdef DBG_ARP_PRITN
    list_t *list = &arp_cache_table.entry_list;
    list_node_t *cur = list->first;

    dbg_info("++++++++++++++++++++++++++show arp cache list++++++++++++++++\r\n");
    dbg_info("list count is:%d\r\n", list->count);
    while (cur)
    {
        arp_entry_t *entry = list_node_parent(cur, arp_entry_t, node);
        arp_show_cache_entry(entry);
        cur = cur->next;
    }
    dbg_info("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
#endif
}
static void arp_cache_table_init(arp_cache_table_t *table)
{
    memset(table, 0, sizeof(arp_cache_table_t));
    list_init(&table->entry_list);
    mempool_init(&table->entry_pool, table->entry_buff, ARP_ENTRY_MAX_SIZE, sizeof(arp_entry_t));
}

arp_entry_t *arp_cache_alloc_entry(arp_cache_table_t *table, int force)
{
    if (!table)
    {
        dbg_error("param false\r\n");
        return NULL;
    }
    arp_entry_t *entry = mempool_alloc_blk(&table->entry_pool, -1);
    if (!entry)
    {
        if (force)
        {
            // 如果必须要分配,把最后那个移除掉
            list_node_t *lastnode = list_remove(&table->entry_list, table->entry_list.last);
            arp_entry_t *new_entry = list_node_parent(lastnode, arp_entry_t, node);
            memset(new_entry, 0, sizeof(arp_entry_t));
            return new_entry;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        memset(entry, 0, sizeof(arp_entry_t));
        return entry;
    }
    return NULL;
}
void arp_cache_free_entry(arp_cache_table_t *table, arp_entry_t *entry)
{
    if (!table || !entry)
    {
        dbg_error("param false\r\n");
        return;
    }
    // 释放pkg缓存
    list_node_t *cur = entry->pkg_list.first;
    while (cur)
    {
        pkg_t *pkg = list_node_parent(cur, pkg_t, node);
        memset(pkg, 0, sizeof(pkg_t));
        package_collect(pkg);
        cur = cur->next;
    }
    // 释放entry
    mempool_free_blk(&table->entry_pool, entry);
}
arp_entry_t *arp_cache_remove_entry(arp_cache_table_t *table, arp_entry_t *entry)
{
    if (!table || !entry)
    {
        dbg_error("param false\r\n");
        return NULL;
    }

    list_node_t *rm_node = list_remove(&table->entry_list, &entry->node);
    arp_entry_t *ret;
    if (rm_node)
    {
        ret = list_node_parent(rm_node, arp_entry_t, node);
        return ret;
    }
    else
    {
        return NULL;
    }
    return NULL;
}
int arp_cache_insert_entry(arp_cache_table_t *table, arp_entry_t *entry)
{
    if (!table || !entry)
    {
        dbg_error("param false\r\n");
        return -1;
    }

    list_insert_first(&table->entry_list, &entry->node);
    return 0;
}
arp_entry_t *arp_cache_find(arp_cache_table_t *table, ipaddr_t *ip)
{
    arp_entry_t *ret;
    list_node_t *cur = table->entry_list.first;
    while (cur)
    {
        arp_entry_t *cur_e = list_node_parent(cur, arp_entry_t, node);
        if (cur_e->ip.q_addr == ip->q_addr)
        {
            ret = cur_e;
            arp_cache_remove_entry(table, cur_e);
            arp_cache_insert_entry(table, cur_e);
            return ret;
        }
        cur = cur->next;
    }
    return NULL;
}

int arp_entry_insert_pkg(arp_entry_t *entry, pkg_t *pkg)
{
    if (!entry || !pkg)
    {
        dbg_error("param false\r\n");
        return -1;
    }

    list_insert_last(&entry->pkg_list, &pkg->node);
}
pkg_t *arp_entry_remove_pkg(arp_entry_t *entry, pkg_t *pkg)
{
    if (!entry || !pkg)
    {
        dbg_error("param false\r\n");
        return NULL;
    }

    list_remove(&entry->pkg_list, &pkg->node);
    return pkg;
}
#include "soft_timer.h"
void arp_cache_scan_period(void *arg)
{
    list_node_t *cur = arp_cache_table.entry_list.first;
    while (cur)
    {
        list_node_t *next = cur->next;
        arp_entry_t *entry = list_node_parent(cur, arp_entry_t, node);
        if (--entry->tmo == 0)
        {
            switch (entry->state)
            {
            case ARP_ENTRY_STATE_RESOLVED:
                ipaddr_t dest_ip;
                dest_ip.q_addr = entry->ip.q_addr;
                entry->tmo = ARP_ENTRY_TMO_RESOLVING;
                entry->retry = ARP_ENTRY_RETRY;
                entry->state = ARP_ENTRY_STATE_WAITING;
                // 重新请求mac地址
                arp_send_request(entry->netif, &dest_ip);
                break;
            case ARP_ENTRY_STATE_WAITING:
                if (--entry->retry == 0)
                {
                    arp_cache_remove_entry(&arp_cache_table, entry);
                    arp_cache_free_entry(&arp_cache_table, entry);
                }
                else
                {
                    ipaddr_t dest_ip;
                    dest_ip.q_addr = entry->ip.q_addr;
                    entry->tmo = ARP_ENTRY_TMO_RESOLVING;
                    entry->state = ARP_ENTRY_STATE_WAITING;
                    // 重新请求mac地址
                    arp_send_request(entry->netif, &dest_ip);
                }
                break;
            default:
                dbg_error("unkown entry state\r\n");
                break;
            }
        }

        cur = next;
    }
}
void arp_init(void)
{
    arp_cache_table_init(&arp_cache_table);
    soft_timer_t *timer = soft_timer_alloc();
    soft_timer_add(timer, SOFT_TIMER_TYPE_PERIOD, 1000, "arp timer", arp_cache_scan_period, NULL, NULL);
}

/*发送arp请求包*/
int arp_send_request(netif_t *netif, const ipaddr_t *dest_ip)
{
    if (!netif || !dest_ip)
    {
        return -1;
    }
    pkg_t *pkg = package_alloc(sizeof(arp_pkg_t));
    if (!pkg)
    {
        dbg_error("create pkg fail\r\n");
        return -1;
    }

    arp_pkg_t *arp_pkg = (arp_pkg_t *)package_data(pkg, sizeof(arp_pkg_t), 0);
    _htons(ARP_HARD_TYPE_ETHER, &arp_pkg->hard_type);
    _htons(ARP_PROTOCAL_TYPE_IPV4, &arp_pkg->protocal_type);
    arp_pkg->hard_len = MAC_ADDR_ARR_LEN;
    arp_pkg->protocal_len = IPV4_ADDR_ARR_LEN;
    _htons(ARP_OPCODE_REQUEST, &arp_pkg->opcode);
    memcpy(arp_pkg->src_mac, netif->macaddr, MAC_ADDR_ARR_LEN);
    _htonl(netif->info.ipaddr.q_addr, &arp_pkg->src_ip);
    mac_s2n(arp_pkg->dest_mac, EMPTY_MAC_ADDR);
    _htonl(dest_ip->q_addr, &arp_pkg->dest_ip);

    uint8_t broadcast_mac[MAC_ADDR_ARR_LEN] = {0};
    mac_s2n(broadcast_mac, BROADCAST_MAC_ADDR);
    int ret = ether_raw_out(netif, PROTOCAL_TYPE_ARP, broadcast_mac, pkg);
    if (ret < 0)
    {
        package_collect(arp_pkg);
        dbg_warning("send an arp pkg fail\r\n");
        return ret;
    }
    return 0;
}

/*发送免费包，开机通知其他主机我的mac是多少*/
int arp_send_no_reply(netif_t *netif)
{
    return arp_send_request(netif, &netif->info.ipaddr);
}

int arp_send_reply(netif_t *netif, pkg_t *pkg)
{
    if (!netif || !pkg)
    {
        return -1;
    }
    arp_pkg_t *arp = (arp_pkg_t *)package_data(pkg, sizeof(arp_pkg_t), 0);
    _htons(ARP_OPCODE_REPLY, &arp->opcode);
    memcpy(arp->dest_mac, arp->src_mac, MAC_ADDR_ARR_LEN);
    arp->dest_ip = arp->src_ip;
    memcpy(arp->src_mac, netif->macaddr, MAC_ADDR_ARR_LEN);
    _htonl(netif->info.ipaddr.q_addr, &arp->src_ip);

    arp_show(arp);
    uint8_t dest_mac[MAC_ADDR_ARR_LEN] = {0};
    memcpy(dest_mac, arp->dest_mac, MAC_ADDR_ARR_LEN);
    int ret = ether_raw_out(netif, PROTOCAL_TYPE_ARP, dest_mac, pkg);
    if (ret < 0)
    {
        package_collect(pkg);
        dbg_warning("send an arp pkg fail\r\n");
        return ret;
    }
    return 0;
}
void arp_show(arp_pkg_t *arp)
{
#ifdef DBG_ARP_PRITN
    uint16_t ht = 0, pt = 0, opc = 0;
    _ntohs(arp->hard_type, &ht);
    _ntohs(arp->protocal_type, &pt);
    _ntohs(arp->opcode, &opc);
    char mac_src_buf[20] = {0};
    char mac_dest_buf[20] = {0};
    mac_n2s(arp->src_mac, mac_src_buf);
    mac_n2s(arp->dest_mac, mac_dest_buf);

    ipaddr_t ip_src_host = {
        .type = IPADDR_V4};
    ipaddr_t ip_dest_host = {
        .type = IPADDR_V4};
    _ntohl(arp->src_ip, ip_src_host.a_addr);
    _ntohl(arp->dest_ip, ip_dest_host.a_addr);

    char ip_src_buf[20] = {0};
    char ip_dest_buf[20] = {0};
    ipaddr_n2s(&ip_src_host, ip_src_buf, 20);
    ipaddr_n2s(&ip_dest_host, ip_dest_buf, 20);

    dbg_info("hard_type:%d\r\n", ht);
    dbg_info("protocal_type:%04x\r\n", pt);
    dbg_info("hard_len:%d\r\n", arp->hard_len);
    dbg_info("protocal_len:%d\r\n", arp->protocal_len);
    dbg_info("opcode:%d\r\n", opc);
    dbg_info("src_mac:%s\r\n", mac_src_buf);
    dbg_info("src_ip:%s\r\n", ip_src_buf);
    dbg_info("dest_mac:%s\r\n", mac_dest_buf);
    dbg_info("dest_ip:%s\r\n", ip_dest_buf);
#endif
}
static is_arp_ok(pkg_t *pkg)
{
    if (pkg->total != sizeof(arp_pkg_t))
    {

        return -1;
    }
    arp_pkg_t *arp = package_data(pkg, sizeof(arp_pkg_t), 0);
    uint16_t ht = 0, pt = 0, opc = 0;
    _ntohs(arp->hard_type, &ht);
    _ntohs(arp->protocal_type, &pt);
    _ntohs(arp->opcode, &opc);
    char mac_src_buf[20] = {0};
    char mac_dest_buf[20] = {0};
    mac_n2s(arp->src_mac, mac_src_buf);
    mac_n2s(arp->dest_mac, mac_dest_buf);

    ipaddr_t ip_src_host = {
        .type = IPADDR_V4};
    ipaddr_t ip_dest_host = {
        .type = IPADDR_V4};
    _ntohl(arp->src_ip, ip_src_host.a_addr);
    _ntohl(arp->dest_ip, ip_dest_host.a_addr);

    char ip_src_buf[20] = {0};
    char ip_dest_buf[20] = {0};
    ipaddr_n2s(&ip_src_host, ip_src_buf, 20);
    ipaddr_n2s(&ip_dest_host, ip_dest_buf, 20);

    if (ht != ARP_HARD_TYPE_ETHER)
    {
        return 0;
    }
    if (pt != ARP_PROTOCAL_TYPE_IPV4)
    {
        return 0;
    }
    if (arp->hard_len != MAC_ADDR_ARR_LEN)
    {
        return 0;
    }
    if (arp->protocal_len != IPV4_ADDR_ARR_LEN)
    {
        return 0;
    }
    if (opc != ARP_OPCODE_REQUEST && opc != ARP_OPCODE_REPLY)
    {
        return 0;
    }

    return 1;
}
void arp_entry_send_all_pkgs(arp_entry_t *entry)
{
    int ret;
    list_node_t *cur = entry->pkg_list.first;
    while (cur)
    {
        list_node_t* next = cur->next;
        pkg_t *pkg = list_node_parent(cur, pkg_t, node);
        arp_entry_remove_pkg(entry,pkg);
        ret = ether_raw_out(entry->netif, PROTOCAL_TYPE_IPV4, entry->mac, pkg);
        if (ret < 0)
        {
            package_collect(pkg);
        }
        cur = next;
    }
}
int arp_in(netif_t *netif, pkg_t *pkg)
{

    if (!is_arp_ok(pkg))
    {
        dbg_error("recv an arp pkg,wrong fomat\r\n");
        return -1;
    }
    arp_pkg_t *arp_pkg = package_data(pkg, sizeof(arp_pkg_t), 0);
    dbg_info("+++++++++++++arp in+++++++++++++++++++++++\r\n");
    arp_show(arp_pkg);
    uint16_t opc = 0;
    uint32_t dest_ip_host = 0;
    uint32_t src_ip_host = 0;
    _ntohl(arp_pkg->dest_ip, &dest_ip_host);
    _ntohl(arp_pkg->src_ip, &src_ip_host);
    _ntohs(arp_pkg->opcode, &opc);
    ipaddr_t src_addr;
    src_addr.type = IPADDR_V4;
    src_addr.q_addr = src_ip_host;
    // 只要目的ip是自己
    if (dest_ip_host == netif->info.ipaddr.q_addr)
    {
        // 不论是请求包还是响应包，都得写入arp_cache_table
        arp_entry_t *entry = arp_cache_find(&arp_cache_table, &src_addr);
        if (entry)
        {
            memcpy(entry->mac, arp_pkg->src_mac, MAC_ADDR_ARR_LEN);
            entry->state = ARP_ENTRY_STATE_RESOLVED;
            entry->tmo = ARP_ENTRY_TMO_STABLE;
            arp_entry_send_all_pkgs(entry);
        }
        else
        {
            // 分配arp cache表项，并加入缓存
            arp_entry_t *new_entry = arp_cache_alloc_entry(&arp_cache_table, 1);
            memset(new_entry, 0, sizeof(arp_entry_t));
            new_entry->ip.q_addr = src_ip_host;
            new_entry->ip.type = IPADDR_V4;
            memcpy(new_entry->mac, arp_pkg->src_mac, MAC_ADDR_ARR_LEN);
            new_entry->state = ARP_ENTRY_STATE_RESOLVED;
            new_entry->netif = netif;
            new_entry->tmo = ARP_ENTRY_TMO_STABLE;
            new_entry->retry = ARP_ENTRY_RETRY;

            arp_cache_insert_entry(&arp_cache_table, new_entry);
        }
        // 如果是请求包
        if (opc == ARP_OPCODE_REQUEST)
        {
            return arp_send_reply(netif, pkg);
        }
    }
    else
    {
        // ARP 无回应包
        arp_entry_t *new_entry = arp_cache_alloc_entry(&arp_cache_table, 0);
        if (new_entry)
        {
            memset(new_entry, 0, sizeof(arp_entry_t));
            new_entry->ip.q_addr = src_ip_host;
            new_entry->ip.type = IPADDR_V4;
            memcpy(new_entry->mac, arp_pkg->src_mac, MAC_ADDR_ARR_LEN);
            new_entry->state = ARP_ENTRY_STATE_RESOLVED;
            new_entry->netif = netif;
            new_entry->tmo = ARP_ENTRY_TMO_STABLE;
            new_entry->retry = ARP_ENTRY_RETRY;

            arp_cache_insert_entry(&arp_cache_table, new_entry);
        }
    }

    package_collect(pkg);
    return 0;
}
