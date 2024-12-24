#include "ipv4.h"
#include "netif.h"
#include "algrithem.h"
#include "package.h"
void ipv4_init(void)
{
    ;
}
static void parse_ipv4_header(const ipv4_header_t *ip_head, ipv4_head_parse_t *parsed)
{
    // 解析字段
    parsed->version = (ip_head->version_and_ihl >> 4) & 0x0F;
    parsed->head_len = (ip_head->version_and_ihl & 0x0F) * 4;

    parsed->dscp = (ip_head->DSCP6_and_ENC2 >> 2) & 0x3F;
    parsed->enc = ip_head->DSCP6_and_ENC2 & 0x03;

    _ntohs(ip_head->total_len, &parsed->total_len);
    _ntohs(ip_head->id, &parsed->id);

    uint16_t frag_flags_and_offset_host = 0;
    // 将网络字节序转换为主机字节序
    _ntohs(ip_head->frag_flags_and_offset, &frag_flags_and_offset_host);
    // 提取 flags 和 frag_offset
    parsed->flags = (frag_flags_and_offset_host >> 13) & 0x07; // 高3位为 flags
    parsed->frag_offset = frag_flags_and_offset_host & 0x1FFF; // 低13位为 frag_offset

    parsed->ttl = ip_head->ttl;
    parsed->protocol = ip_head->protocal;

    _ntohs(ip_head->h_checksum, &parsed->checksum);
    _ntohl(ip_head->src_ip, &parsed->src_ip);
    _ntohl(ip_head->dest_ip, &parsed->dest_ip);
}

static int ipv4_pkg_is_ok(ipv4_head_parse_t* head,ipv4_header_t *ip_head)
{
    if(head->version!=IPV4_HEAD_VERSION)
    {
        dbg_warning("ipv4 pkg format may be wrong\r\n");
        return 0;
    }
    if(head->head_len < IPV4_HEAD_MIN_SIZE || head->head_len > IPV4_HEAD_MAX_SIZE)
    {
        dbg_warning("ipv4 pkg format may be wrong\r\n");
        return 0;
    }
    if(head->head_len != sizeof(ipv4_header_t))
    {
        dbg_warning("ipv4 pkg format may be wrong\r\n");
        return 0;
    }
    if(head->total_len < head->head_len)
    {
        dbg_warning("ipv4 pkg format may be wrong\r\n");
        return 0;
    }
    
    // 校验和为0时，即为不需要检查检验和
    if (head->checksum) {
        uint16_t c = checksum16(0,(uint16_t*)ip_head, head->head_len, 0, 1);
        if (c != 0) {
            dbg_warning("Bad checksum: %0x(correct is: %0x)\n", head->checksum, c);
            return 0;
        }
    }


    return 1;

}
int ipv4_in(netif_t *netif, pkg_t *pkg)
{
    if (!netif || !pkg)
    {
        dbg_error("param fault\r\n");
        return -1;
    }

    ipv4_header_t *ip_head = (ipv4_header_t *)package_data(pkg, sizeof(ipv4_header_t), 0);
    ipv4_head_parse_t parse_head;
    parse_ipv4_header(ip_head,&parse_head);

    if(!ipv4_pkg_is_ok(&parse_head,ip_head))
    {
        dbg_warning(" a ipv4 pkg is not ok\r\n");
        return -1;
    }
    dbg_info("++++++++IPV4 in+++++++++++++++++++++++\r\n");

    if(pkg->total > parse_head.total_len)
    {
        //ether 最小字节数46，可能自动填充了一些0
        package_shrank_last(pkg,pkg->total-parse_head.total_len);
    }
}