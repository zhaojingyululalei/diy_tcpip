
#include "icmpv4.h"
#include "ipv4.h"
#include "protocal.h"
static int is_icmpv4_pkg_ok(icmpv4_pkt_t *icmp, pkg_t *pkg)
{
    if (pkg->total < sizeof(icmpv4_head_t))
    {
        dbg_warning("icmp format fault:size\r\n");
        return 0;
    }

    uint16_t checksum = package_checksum16(pkg, 0, pkg->total, 0, 1);
    if (checksum != 0)
    {
        dbg_warning("icmp format fault:checksum\r\n");
        return 0;
    }

    return 1;
}

int icmpv4_send_reply(ipaddr_t* src,ipaddr_t* dest,pkg_t* pkg)
{
    int ret;
    icmpv4_head_t* head = package_data(pkg,sizeof(icmpv4_head_t),0);
    head->checksum = 0;
    head->type = ICMPv4_ECHO_REPLY;
    ret = icmpv4_out(src,dest,pkg);
    return ret;
}
int icmpv4_send_unreach(ipaddr_t *src, ipaddr_t *dest, pkg_t *pkg, int code)
{
    int ret;
    package_add_headspace(pkg,sizeof(icmpv4_pkt_t));
    icmpv4_pkt_t* icmp_head = package_data(pkg,sizeof(icmpv4_pkt_t),0);
    icmp_head->hdr.type = ICMPv4_UNREACH;
    icmp_head->hdr.code = code;
    icmp_head->hdr.checksum = 0;
    icmp_head->reverse = 0;

    ret = icmpv4_out(src,dest,pkg);
    return ret;
}
void icmpv4_init(void)
{
    ;
}

int icmpv4_in(ipaddr_t *src, ipaddr_t *host, pkg_t *pkg, int ipv4_head_len)
{
    if (!pkg)
    {
        dbg_error("pkg is null\r\n");
        return -1;
    }
    int ret = 0;

    // 保存ipv4头
    ipv4_header_t ipv4_head = *((ipv4_header_t *)package_data(pkg, ipv4_head_len, 0));
    // 除去数据包中的ipv4头
    package_shrank_front(pkg, ipv4_head_len);

    icmpv4_pkt_t *icmpv4_pkg = (icmpv4_pkt_t *)package_data(pkg, sizeof(icmpv4_pkt_t), 0);
    if (!is_icmpv4_pkg_ok(icmpv4_pkg, pkg))
    {
        dbg_warning("icmpv4 format fault\r\n");
        return -1;
    }

    switch (icmpv4_pkg->hdr.type)
    {
    case ICMPv4_ECHO_REQUEST:
        ret = icmpv4_send_reply(host,src,pkg);
        if(ret < 0)
        {
            return ret;
        }
        break;
    
    default:
        break;
    }
}

int icmpv4_out(ipaddr_t* src,ipaddr_t* dest,pkg_t* pkg)
{
    int ret;
    icmpv4_head_t* head = package_data(pkg,sizeof(icmpv4_head_t),0);
    head->checksum = package_checksum16(pkg,0,pkg->total,0,1);
    ret = ipv4_out(pkg,PROTOCAL_TYPE_ICMPV4,src,dest);
    return ret;

}