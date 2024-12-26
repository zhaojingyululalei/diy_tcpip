#include "ipv4.h"
#include "netif.h"
#include "algrithem.h"
#include "package.h"
#include "protocal.h"

void parse_ipv4_header(const ipv4_header_t *ip_head, ipv4_head_parse_t *parsed)
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

static int ipv4_pkg_is_ok(ipv4_head_parse_t *head, ipv4_header_t *ip_head)
{
    if (head->version != IPV4_HEAD_VERSION)
    {
        dbg_warning("ipv4 pkg format may be wrong\r\n");
        return 0;
    }
    if (head->head_len < IPV4_HEAD_MIN_SIZE || head->head_len > IPV4_HEAD_MAX_SIZE)
    {
        dbg_warning("ipv4 pkg format may be wrong\r\n");
        return 0;
    }
    if (head->head_len != sizeof(ipv4_header_t))
    {
        dbg_warning("ipv4 pkg format may be wrong\r\n");
        return 0;
    }
    if (head->total_len < head->head_len)
    {
        dbg_warning("ipv4 pkg format may be wrong\r\n");
        return 0;
    }

    // 校验和为0时，即为不需要检查检验和
    if (head->checksum)
    {
        uint16_t c = checksum16(0, (uint16_t *)ip_head, head->head_len, 0, 1);
        if (c != 0)
        {
            dbg_warning("Bad checksum: %0x(correct is: %0x)\n", head->checksum, c);
            return 0;
        }
    }

    return 1;
}
static int ipv4_is_match(netif_t *netif, uint32_t dest_ip)
{
    // 本机ip，局部广播,255.255.255.255 都是匹配的
    ipaddr_t dest = {
        .type = IPADDR_V4,
        .q_addr = dest_ip};

    // 是否是255.255.255.255
    if (is_global_boradcast(&dest))
    {
        return 1;
    }
    // 看看主机部分是否为全1
    if (is_local_boradcast(netif, &dest))
    {
        return 1;
    }
    if (netif->info.ipaddr.q_addr == dest.q_addr)
    {
        return 1;
    }
    return 0;
}
static int ipv4_normal_in(netif_t *netif, pkg_t *pkg, ipv4_head_parse_t *parse_head)
{
    switch (parse_head->protocol)
    {
    case PROTOCAL_TYPE_ICMPV4:

        break;
    case PROTOCAL_TYPE_UDP:

        break;
    case PROTOCAL_TYPE_TCP:

        break;

    default:
        dbg_warning("ipv4 normal in unkown protocal:%d\r\n", parse_head->protocol);
        return -1;
    }
    return 0;
}
static void ipv4_show_pkg(ipv4_head_parse_t *parse)
{
#ifdef DBG_IPV4_PRINT
    dbg_info("++++++++++++++++++++show ipv4 header++++++++++++++++\r\n");
    dbg_info("version:%d\r\n", parse->version);
    dbg_info("head_len:%d\r\n", parse->head_len);
    dbg_info("dscp:%d\r\n", parse->dscp);
    dbg_info("enc:%d\r\n", parse->enc);
    dbg_info("total_len:%d\r\n", parse->total_len);
    dbg_info("id:%x\r\n", parse->id);
    dbg_info("flags_1,Reserved bit:%x\r\n", (parse->flags & 0x04) ? 1 : 0);
    dbg_info("flags_2,Dont fragment:%x\r\n", (parse->flags & 0x02) ? 1 : 0);
    dbg_info("flags_3,More fragment:%x\r\n", (parse->flags & 0x01) ? 1 : 0);
    dbg_info("frag_offset:%d\r\n", parse->frag_offset);
    dbg_info("ttl:%d\r\n", parse->ttl);
    dbg_info("protocal:%02x\r\n", parse->protocol);
    dbg_info("checksum:%x\r\n", parse->checksum);
    ipaddr_t src, dest;
    src.type = IPADDR_V4;
    src.q_addr = parse->src_ip;
    dest.type = IPADDR_V4;
    dest.q_addr = parse->dest_ip;
    char src_buf[20] = {0};
    char dest_buf[20] = {0};
    ipaddr_n2s(&src, src_buf, 20);
    ipaddr_n2s(&dest, dest_buf, 20);
    dbg_info("src_ip:%s\r\n", src_buf);
    dbg_info("dest_ip:%s\r\n", dest_buf);
    dbg_info("++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
#endif
}

void ipv4_set_header(const ipv4_head_parse_t *parsed, ipv4_header_t *head)
{
    if (parsed == NULL || head == NULL)
    {
        return; // 防止空指针异常
    }

    // 将 version 和 head_len 合并到 version_and_ihl
    head->version_and_ihl = ((parsed->version & 0x0F) << 4) | ((parsed->head_len / 4) & 0x0F);

    // DSCP 和 ENC2
    head->DSCP6_and_ENC2 = ((parsed->dscp & 0x3F) << 2) | (parsed->enc & 0x03);

    // 总长度（转换为网络字节序）
    _htons(parsed->total_len, &head->total_len);

    // 标识字段（转换为网络字节序）
    _htons(parsed->id, &head->id);

    // flags 和 fragment offset 合并后转换为网络字节序
    uint16_t frag_flags_and_offset_host = ((parsed->flags & 0x07) << 13) | (parsed->frag_offset & 0x1FFF);
    _htons(frag_flags_and_offset_host, &head->frag_flags_and_offset);

    // TTL 和协议
    head->ttl = parsed->ttl;
    head->protocal = parsed->protocol;

    // 校验和（转换为网络字节序）
    _htons(parsed->checksum, &head->h_checksum);

    // 源地址和目标地址（转换为网络字节序）
    _htonl(parsed->src_ip, &head->src_ip);
    _htonl(parsed->dest_ip, &head->dest_ip);
}

int ipv4_in(netif_t *netif, pkg_t *pkg)
{
    if (!netif || !pkg)
    {
        dbg_error("param fault\r\n");
        return -1;
    }
    int ret;

    ipv4_header_t *ip_head = (ipv4_header_t *)package_data(pkg, sizeof(ipv4_header_t), 0);
    ipv4_head_parse_t parse_head;
    parse_ipv4_header(ip_head, &parse_head);

    

    if (!ipv4_pkg_is_ok(&parse_head, ip_head))
    {
        dbg_warning(" a ipv4 pkg is not ok\r\n");
        return -1;
    }
    dbg_info("++++++++IPV4 in+++++++++++++++++++++++\r\n");

    if (pkg->total > parse_head.total_len)
    {
        // ether 最小字节数46，可能自动填充了一些0
        package_shrank_last(pkg, pkg->total - parse_head.total_len);
    }

    if (!ipv4_is_match(netif, parse_head.dest_ip))
    {
        dbg_warning("recv an ipv4 pkg,dest ip not match\r\n");
        return -2;
    }
    ipv4_show_pkg(&parse_head);

    // 不分片ipv4数据包 处理
    ret = ipv4_normal_in(netif, pkg, &parse_head);
    return ret;
}

int ipv4_out(pkg_t* pkg,uint8_t protocal,ipaddr_t* src,ipaddr_t* dest)
{

    int ret;
    package_add_headspace(pkg,sizeof(ipv4_header_t));
    ipv4_header_t* head = package_data(pkg,sizeof(ipv4_header_t),0);
    package_memset(pkg,0,0,sizeof(ipv4_header_t));
    ipv4_head_parse_t parse ;
    memset(&parse,0,sizeof(ipv4_head_parse_t));

    parse.version = IPV4_HEAD_VERSION;
    parse.head_len = sizeof(ipv4_header_t);
    parse.total_len = pkg->total;
    parse.flags = IPV4_HEAD_FLAGS_NOT_FRAGMENT;
    parse.ttl = IPV4_HEAD_TTL_DEFAULT;
    parse.protocol = protocal;
    parse.checksum = 0;
    parse.src_ip = src->q_addr;
    parse.dest_ip = dest->q_addr;

    ipv4_set_header(&parse,head);
    //计算校验值
    uint16_t check_ret = checksum16(0, (uint16_t *)head, sizeof(ipv4_header_t), 0, 1);
    //这里直接赋值，不要大小端转换  解析的时候，checksum转不转换都行
    head->h_checksum = check_ret;


    //选择从哪块网卡发出数据包
    netif_t* out_card = NULL;
    ipaddr_t loop_ip;
    ipaddr_s2n(NETIF_LOOP_IPADDR,&loop_ip);
    //目的地址是回环接口，没有链路层，不用从物理网卡发出
    if(dest->q_addr == loop_ip.q_addr)
    {
        //根据dest ip找到网卡
        out_card = get_netif_accord_ip(dest);
        if(!out_card)
        {
            dbg_warning("loop not open,can not send pkg from loop\r\n");
            package_collect(pkg);
            return -1;
        }
        else{
            netif_out(out_card,dest,pkg);
        }
    }
    else{
        //根据src ip找物理网卡
        out_card = get_netif_accord_ip(src);
        if(!out_card)
        {
            dbg_warning("src_ip relate netif not open,can not send pkg from loop\r\n");
            package_collect(pkg);
            return -1;
        }
        else{
            netif_out(out_card,dest,pkg);
        }
    }
    return 0;
}

void ipv4_init(void)
{
    ;
}