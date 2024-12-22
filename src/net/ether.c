#include "ether.h"
#include "netif.h"
#include "protocal.h"
#include "networker.h"
#include "arp.h"
static int(ether_open)(struct _netif_t *netif)
{
    int ret ;
    //发送一个无回报arp包，
    //1.告诉别人自己的mac，便于让别人更新arp缓存
    //2.探测ip地址是否冲突
    ret = arp_send_no_reply(netif);
    if(ret <0)
    {
        return ret;
    }
    return 0;
}
static void(ether_close)(struct _netif_t *netif)
{
    return;
}
static int ether_pkg_is_ok(ether_header_t *header, int pkg_len)
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
    mac_n2s(header->dest,destbuf);
    mac_n2s(header->src,srcbuf);
    uint16_t pro_type = 0;
    n2h(&header->protocal,2,&pro_type);//type占2字节
    dbg_info("dest_mac:%s\r\n",destbuf);
    dbg_info("src_mac:%s\r\n",srcbuf);
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
static int(ether_in)(struct _netif_t *netif, pkg_t *package)
{
    int ret;

    ether_header_t *header = package_data(package, sizeof(ether_header_t), 0);
    if (ether_pkg_is_ok(header, package->total) < 0)
    {
        dbg_warning("ether pkg problem,can not handle\r\n");
        return -1;
    }

    ether_dbg_print_pkg(package);
    uint16_t protocal = 0;
    _ntohs(header->protocal,&protocal);
    switch (protocal)
    {
    case PROTOCAL_TYPE_ARP:
        //去掉以太网头
        ret = package_shrank_front(package,sizeof(ether_header_t));
        if(ret < 0)
        {
            dbg_error("package_shrank_fail\r\n");
            return ret;
        }
        ret = arp_in(netif,package);
        if(ret < 0)
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


int ether_raw_out(netif_t* netif,protocal_type_t type,const uint8_t* mac_dest,pkg_t* pkg)
{
    int ret;
    int size = pkg->total;
    if(size < MTU_MIN_SIZE)
    {
        //不足46字节，调整至46字节
        package_expand_last(pkg,MTU_MIN_SIZE-size);
        package_memset(pkg,size,0,MTU_MIN_SIZE-size);
    }

    ret = package_add_headspace(pkg,sizeof(ether_header_t));
    if(ret<0)
    {
        dbg_error("pkg add header fail\r\n");
        return ret;
    }
    //填充以太网数据包头
    ether_header_t* head = package_data(pkg,sizeof(ether_header_t),0);
    memcpy(head->src,netif->macaddr,MAC_ADDR_ARR_LEN);
    memcpy(head->dest,mac_dest,MAC_ADDR_ARR_LEN);
    h2n(&type,2,&head->protocal);

    ether_dbg_print_pkg(pkg);//打印包信息

    netif_t* target = is_mac_host(mac_dest);//检测目标网卡是不是主机网卡
    if(target)
    {
        //如果是主机上的网卡，并且还是激活状态
        if(target->state==NETIF_ACTIVe)
        {
            ret = netif_putpkg(&target->in_q,pkg,-1);//直接把包放入对应的输入队列
            if(ret==0)
            {
                post(&mover_sem); //放入成功，唤醒搬运线程搬运
            }
        }
    }
    else{
        //直接放到本网卡输出队列，等待发送
        netif_putpkg(&netif->out_q,pkg,-1);
    }


}
static int(ether_out)(struct _netif_t *netif, ipaddr_t *dest, pkg_t *package)
{
    int ret;
    netif_t* target = is_ip_host(dest);
    if(target)
    {
        if(target->state == NETIF_ACTIVe)
        {
            ret = ether_raw_out(target,PROTOCAL_TYPE_IPV4,target->macaddr,package);
            if(ret < 0)
            {
                return ret;
            }
        }
    }
    else
    {
        //非本机ip
        uint8_t broad_cast[MAC_ADDR_ARR_LEN]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        ether_raw_out(netif,PROTOCAL_TYPE_IPV4,broad_cast,package);
    }
    return 0;
}
static const link_layer_t link_layer = {
    .type = NETIF_TYPE_ETH,
    .open = ether_open,
    .close = ether_close,
    .in = ether_in,
    .out = ether_out,
};
void ether_init(void)
{
    netif_register_link_layer(&link_layer);
}

