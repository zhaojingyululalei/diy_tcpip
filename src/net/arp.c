#include "arp.h"
#include "ether.h"
static arp_cache_table_t arp_cache_table;

static void arp_cache_table_init(arp_cache_table_t* table)
{
    memset(table,0,sizeof(arp_cache_table_t));
    list_init(&table->entry_list);
    mempool_init(&table->entry_pool,table->entry_buff,ARP_ENTRY_MAX_SIZE,sizeof(arp_entry_t));
}

void arp_init(void)
{
    arp_cache_table_init(&arp_cache_table);
}

/*发送arp请求包*/
int arp_send_request(netif_t* netif, const ipaddr_t* dest_ip)
{
    if(!netif||!dest_ip)
    {
        return -1;
    }
    pkg_t* pkg = package_alloc(sizeof(arp_pkg_t));
    if(!pkg)
    {
        dbg_error("create pkg fail\r\n");
        return -1;
    }

    arp_pkg_t* arp_pkg = (arp_pkg_t*)package_data(pkg,sizeof(arp_pkg_t),0);
    _htons(ARP_HARD_TYPE_ETHER,&arp_pkg->hard_type);
    _htons(ARP_PROTOCAL_TYPE_IPV4,&arp_pkg->protocal_type);
    arp_pkg->hard_len = MAC_ADDR_ARR_LEN;
    arp_pkg->protocal_len = IPV4_ADDR_ARR_LEN;
    _htons(ARP_OPCODE_REQUEST,&arp_pkg->opcode);
    memcpy(arp_pkg->src_mac,netif->macaddr,MAC_ADDR_ARR_LEN);
    _htonl(netif->info.ipaddr.q_addr,&arp_pkg->src_ip);
    mac_s2n(arp_pkg->dest_mac,EMPTY_MAC_ADDR);
    _htonl(dest_ip->q_addr,&arp_pkg->dest_ip);

    uint8_t broadcast_mac[MAC_ADDR_ARR_LEN] = {0};
    mac_s2n(broadcast_mac,BROADCAST_MAC_ADDR);
    int ret = ether_raw_out(netif,PROTOCAL_TYPE_ARP,broadcast_mac,pkg);
    if(ret < 0)
    {
        package_collect(arp_pkg);
        dbg_warning("send an arp pkg fail\r\n");
        return ret;
    }
    return 0;

}

/*发送免费包，开机通知其他主机我的mac是多少*/
int arp_send_no_reply(netif_t* netif)
{
    return arp_send_request(netif,&netif->info.ipaddr);
}

int arp_send_reply(netif_t* netif,pkg_t* pkg)
{
    if(!netif||!pkg)
    {
        return -1;
    }
    arp_pkg_t* arp = (arp_pkg_t*)package_data(pkg,sizeof(arp_pkg_t),0);
    _htons(ARP_OPCODE_REPLY,&arp->opcode);
    memcpy(arp->dest_mac,arp->src_mac,MAC_ADDR_ARR_LEN);
    arp->dest_ip = arp->src_ip;
    memcpy(arp->src_mac,netif->macaddr,MAC_ADDR_ARR_LEN);
    _htonl(netif->info.ipaddr.q_addr,&arp->src_ip);

    arp_show(arp);
    uint8_t dest_mac[MAC_ADDR_ARR_LEN] = {0};
    memcpy(dest_mac,arp->dest_mac,MAC_ADDR_ARR_LEN);
    int ret = ether_raw_out(netif,PROTOCAL_TYPE_ARP,dest_mac,pkg);
    if(ret < 0)
    {
        package_collect(pkg);
        dbg_warning("send an arp pkg fail\r\n");
        return ret;
    }
    return 0;
    
}
void arp_show(arp_pkg_t* arp)
{
#ifdef DBG_ARP_PRITN
    uint16_t ht = 0,pt=0,opc=0;
    _ntohs(arp->hard_type,&ht);
    _ntohs(arp->protocal_type,&pt);
    _ntohs(arp->opcode,&opc);
    char mac_src_buf[20] = {0};
    char mac_dest_buf[20] = {0};
    mac_n2s(arp->src_mac,mac_src_buf);
    mac_n2s(arp->dest_mac,mac_dest_buf);

    ipaddr_t ip_src_host={
        .type = IPADDR_V4
    };
    ipaddr_t ip_dest_host = {
        .type = IPADDR_V4
    };
    _ntohl(arp->src_ip,ip_src_host.a_addr);
    _ntohl(arp->dest_ip,ip_dest_host.a_addr);

    char ip_src_buf[20] = {0};
    char ip_dest_buf[20] = {0};
    ipaddr_n2s(&ip_src_host,ip_src_buf,20);
    ipaddr_n2s(&ip_dest_host,ip_dest_buf,20);

    dbg_info("hard_type:%d\r\n",ht);
    dbg_info("protocal_type:%04x\r\n",pt);
    dbg_info("hard_len:%d\r\n",arp->hard_len);
    dbg_info("protocal_len:%d\r\n",arp->protocal_len);
    dbg_info("opcode:%d\r\n",opc);
    dbg_info("src_mac:%s\r\n",mac_src_buf);
    dbg_info("src_ip:%s\r\n",ip_src_buf);
    dbg_info("dest_mac:%s\r\n",mac_dest_buf);
    dbg_info("dest_ip:%s\r\n",ip_dest_buf);
#endif
}
static is_arp_ok(pkg_t* pkg)
{
    if(pkg->total!=sizeof(arp_pkg_t))
    {
        
        return -1;
    }
    arp_pkg_t *arp = package_data(pkg,sizeof(arp_pkg_t),0);
    uint16_t ht = 0,pt=0,opc=0;
    _ntohs(arp->hard_type,&ht);
    _ntohs(arp->protocal_type,&pt);
    _ntohs(arp->opcode,&opc);
    char mac_src_buf[20] = {0};
    char mac_dest_buf[20] = {0};
    mac_n2s(arp->src_mac,mac_src_buf);
    mac_n2s(arp->dest_mac,mac_dest_buf);

    ipaddr_t ip_src_host={
        .type = IPADDR_V4
    };
    ipaddr_t ip_dest_host = {
        .type = IPADDR_V4
    };
    _ntohl(arp->src_ip,ip_src_host.a_addr);
    _ntohl(arp->dest_ip,ip_dest_host.a_addr);

    char ip_src_buf[20] = {0};
    char ip_dest_buf[20] = {0};
    ipaddr_n2s(&ip_src_host,ip_src_buf,20);
    ipaddr_n2s(&ip_dest_host,ip_dest_buf,20);

    if(ht!=ARP_HARD_TYPE_ETHER)
    {
        return 0;
    }
    if(pt!=ARP_PROTOCAL_TYPE_IPV4)
    {
        return 0;
    }
    if(arp->hard_len!=MAC_ADDR_ARR_LEN)
    {
        return 0;
    }
    if(arp->protocal_len!= IPV4_ADDR_ARR_LEN)
    {
        return 0;
    }
    if(opc!=ARP_OPCODE_REQUEST && opc!=ARP_OPCODE_REPLY)
    {
        return 0;
    }

    return  1;
    
}
int arp_in(netif_t* netif,pkg_t* pkg)
{

    if(!is_arp_ok(pkg))
    {
        dbg_error("recv an arp pkg,wrong fomat\r\n");
        return -1;
    }
    arp_pkg_t *arp_pkg = package_data(pkg,sizeof(arp_pkg_t),0);
    dbg_info("handling an arp pkg+++++++++++++++++++++++\r\n");
    arp_show(arp_pkg);
    uint16_t opc = 0;
    uint32_t dest_ip_host = 0;
    _ntohl(arp_pkg->dest_ip,&dest_ip_host);
    _ntohs(arp_pkg->opcode,&opc);
    if(opc==ARP_OPCODE_REQUEST && dest_ip_host==netif->info.ipaddr.q_addr)
    {
        return arp_send_reply(netif,pkg);
    }


    package_collect(pkg);
    return 0;
}