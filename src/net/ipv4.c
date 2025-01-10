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
    parsed->flags = (frag_flags_and_offset_host >> 13) & 0x07;       // 高3位为 flags
    parsed->frag_offset = (frag_flags_and_offset_host & 0x1FFF) * 8; // 低13位为 frag_offset

    parsed->ttl = ip_head->ttl;
    parsed->protocol = ip_head->protocal;

    _ntohs(ip_head->h_checksum, &parsed->checksum);
    _ntohl(ip_head->src_ip, &parsed->src_ip);
    _ntohl(ip_head->dest_ip, &parsed->dest_ip);
}

int ipv4_pkg_is_ok(ipv4_head_parse_t *head, ipv4_header_t *ip_head)
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
#include "icmpv4.h"

static int ipv4_normal_in(netif_t *netif, pkg_t *pkg, ipv4_head_parse_t *parse_head)
{
    int ret;
    ipaddr_t src_ip = {
        .type = IPADDR_V4,
        .q_addr = parse_head->src_ip};
    switch (parse_head->protocol)
    {
    case PROTOCAL_TYPE_ICMPV4:

        ret = icmpv4_in(&src_ip, &netif->info.ipaddr, pkg, parse_head->head_len);
        if (ret < 0)
        {
            dbg_warning("icmpv4 in pkg fault\r\n");
            return ret;
        }
        break;
    case PROTOCAL_TYPE_UDP:

        ret = icmpv4_send_unreach(&netif->info.ipaddr, &src_ip, pkg, ICMPv4_UNREACH_PORT);
        if (ret < 0)
        {
            dbg_warning("icmpv4_send_unreach fail\r\n");
            return ret;
        }
    case PROTOCAL_TYPE_TCP:

        break;

    default:
        dbg_warning("ipv4 normal in unkown protocal:%d\r\n", parse_head->protocol);
        return -1;
    }
    return 0;
}
static int ipv4_frag_in(netif_t *netif, pkg_t *pkg, ipv4_head_parse_t *parse_head)
{
    int ret;
    ipaddr_t src_ip = {
        .type = IPADDR_V4,
        .q_addr = parse_head->src_ip};
    ip_frag_t *frag = ipv4_frag_find(&src_ip, parse_head->id);
    if (!frag)
    {
        frag = ipv4_frag_alloc();
        frag->id = parse_head->id;
        frag->ip.q_addr = parse_head->src_ip;
        frag->ip.type = IPADDR_V4;
    }

    ret = ipv4_frag_insert_pkg(frag, pkg, parse_head);
    if (ret < 0)
    {
        return ret;
    }
    ipv4_frag_list_print();

    if (ipv4_frag_is_all_arrived(frag))
    {
        // ip头总长度，校验和还没改
        pkg_t *pkg = ipv4_frag_join_pkg(frag);
        //释放frag
        ipv4_frag_free(frag);
        ipv4_header_t *head = package_data(pkg, sizeof(ipv4_header_t), 0);
        ipv4_head_parse_t parse;
        parse_ipv4_header(head, &parse);
        ipv4_normal_in(netif, pkg, &parse);
    }
    return 0;
}
static int ipv4_frag_global_id = 1;
static int ipv4_frag_out(netif_t *netif, pkg_t *pkg, uint8_t protocal, ipaddr_t *src, ipaddr_t *dest)
{
    int ret;
    int remain_size = pkg->total;
    int cur_pos = 0;
    int old_pos = 0;
    pkg_t *frag_pkg = NULL;
    while (remain_size)
    {
        if (remain_size > netif->mtu)
        {
            frag_pkg = package_alloc(netif->mtu);
            if (!frag_pkg)
            {
                dbg_error("alloc pkg fail\r\n");
                return -1;
            }
            int cpy_size = netif->mtu - sizeof(ipv4_header_t);
            ret = package_memcpy(frag_pkg, sizeof(ipv4_header_t), pkg, cur_pos, cpy_size);
            if (ret < 0)
            {
                dbg_error("pkg copy fail\r\n");
                package_collect(frag_pkg);
                return -2;
            }
            package_print(frag_pkg);
            old_pos = cur_pos;
            cur_pos += cpy_size;
            remain_size -= cpy_size;
        }
        else
        {
            frag_pkg = package_alloc(remain_size + sizeof(ipv4_header_t));
            if (!frag_pkg)
            {
                dbg_error("alloc pkg fail\r\n");
                return -1;
            }
            int cpy_size = remain_size;
            ret = package_memcpy(frag_pkg, sizeof(ipv4_header_t), pkg, cur_pos, cpy_size);
            if (ret < 0)
            {
                dbg_error("pkg copy fail\r\n");
                package_collect(frag_pkg);
                return -2;
            }
            package_print(frag_pkg);
            old_pos = cur_pos;
            cur_pos += cpy_size;
            remain_size -= cpy_size;
        }

        // 填充头部
        ipv4_header_t *head = package_data(frag_pkg, sizeof(ipv4_header_t), 0);
        package_memset(frag_pkg, 0, 0, sizeof(ipv4_header_t));
        ipv4_head_parse_t parse;
        memset(&parse, 0, sizeof(ipv4_head_parse_t));

        parse.version = IPV4_HEAD_VERSION;
        parse.head_len = sizeof(ipv4_header_t);
        parse.total_len = frag_pkg->total;
        parse.id = ipv4_frag_global_id;
        parse.flags = remain_size > 0 ? IPV4_HEAD_FLAGS_MORE_FRAGMENT : 0;
        parse.frag_offset = old_pos >> 3;

        parse.ttl = IPV4_HEAD_TTL_DEFAULT;
        parse.protocol = protocal;
        parse.checksum = 0;
        parse.src_ip = src->q_addr;
        parse.dest_ip = dest->q_addr;

        ipv4_set_header(&parse, head);
        uint16_t check_ret = package_checksum16(frag_pkg, 0, sizeof(ipv4_header_t), 0, 1);
        // 这里直接赋值，不要大小端转换  解析的时候，checksum转不转换都行
        head->h_checksum = check_ret;
        package_print(frag_pkg);
        ret = netif_out(netif, dest, frag_pkg);
        if (ret < 0)
        {
            dbg_warning("the frag_pkg send fail\r\n");
            package_collect(frag_pkg);
            return -3;
        }
    }
    ipv4_frag_global_id++;
    package_collect(pkg);
    return 0;
}
#include "soft_timer.h"
static soft_timer_t ipv4_frag_timer;

void ipv4_show_pkg(ipv4_head_parse_t *parse)
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

    // 解析包头
    ipv4_header_t *ip_head = (ipv4_header_t *)package_data(pkg, sizeof(ipv4_header_t), 0);
    ipv4_head_parse_t parse_head;
    parse_ipv4_header(ip_head, &parse_head);

    // 检测包头格式
    if (!ipv4_pkg_is_ok(&parse_head, ip_head))
    {
        dbg_warning(" a ipv4 pkg is not ok\r\n");
        return -1;
    }
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
    dbg_info("++++++++IPV4 in+++++++++++++++++++++++\r\n");

    ipv4_show_pkg(&parse_head);

    if (parse_head.flags & 0x1 || parse_head.frag_offset)
    {
        ret = ipv4_frag_in(netif, pkg, &parse_head);
    }
    else
    {
        // 不分片ipv4数据包 处理
        ret = ipv4_normal_in(netif, pkg, &parse_head);
    }

    return ret;
}

int ipv4_out(pkg_t *pkg, uint8_t protocal, ipaddr_t *src, ipaddr_t *dest)
{
    int ret;
    // 选择从哪块网卡发出数据包
    // 这块内容后续由路由表替代，路由表会决定数据包从哪个接口发出
    netif_t *out_card = NULL;
    ipaddr_t loop_ip;
    ipaddr_s2n(NETIF_LOOP_IPADDR, &loop_ip);
    // 目的地址是回环接口，没有链路层，不用从物理网卡发出
    if (dest->q_addr == loop_ip.q_addr)
    {
        // 根据dest ip找到网卡
        out_card = get_netif_accord_ip(dest);
        if (!out_card)
        {
            dbg_warning("loop not open,can not send pkg from loop\r\n");
            package_collect(pkg);
            return -1;
        }
    }
    else
    {
        // 根据src ip找物理网卡
        out_card = get_netif_accord_ip(src);
        if (!out_card)
        {
            dbg_warning("src_ip relate netif not open,can not send pkg from loop\r\n");
            package_collect(pkg);
            return -1;
        }
    }

    if (out_card->mtu == 0)
    {
        dbg_error("the out card mtu not set\r\n");
        return -2;
    }
    // 如果包太大，需要分片发送
    if (pkg->total + sizeof(ipv4_header_t) > out_card->mtu)
    {
        package_print(pkg);
        ret = ipv4_frag_out(out_card, pkg, protocal, src, dest);
        if (ret < 0)
        {
            package_collect(pkg);
            return ret;
        }
    }
    else//否则不用分片发送
    {
        package_add_headspace(pkg, sizeof(ipv4_header_t));
        ipv4_header_t *head = package_data(pkg, sizeof(ipv4_header_t), 0);
        package_memset(pkg, 0, 0, sizeof(ipv4_header_t));
        ipv4_head_parse_t parse;
        memset(&parse, 0, sizeof(ipv4_head_parse_t));

        parse.version = IPV4_HEAD_VERSION;
        parse.head_len = sizeof(ipv4_header_t);
        parse.total_len = pkg->total;
        parse.flags = IPV4_HEAD_FLAGS_NOT_FRAGMENT;
        parse.ttl = IPV4_HEAD_TTL_DEFAULT;
        parse.protocol = protocal;
        parse.checksum = 0;
        parse.src_ip = src->q_addr;
        parse.dest_ip = dest->q_addr;

        ipv4_set_header(&parse, head);
        // 计算校验值
        // uint16_t check_ret = checksum16(0, (uint16_t *)head, sizeof(ipv4_header_t), 0, 1);

        uint16_t check_ret = package_checksum16(pkg, 0, sizeof(ipv4_header_t), 0, 1);
        // 这里直接赋值，不要大小端转换  解析的时候，checksum转不转换都行
        head->h_checksum = check_ret;

        netif_out(out_card, dest, pkg);
        return 0;
    }
}
#include "mmpool.h"
static uint8_t frag_buff[IPV4_FRAG_MAX * (sizeof(list_node_t) + sizeof(ip_frag_t))];
static mempool_t ip_frag_pool;
static list_t ip_frag_list;

void* frag_tmo_handle(void* arg)
{
    list_t* frag_list = &ip_frag_list;
    list_node_t* cur = frag_list->first;
    while(cur)
    {
        list_node_t* next = cur->next;
        ip_frag_t* frag = list_node_parent(cur,ip_frag_t,node);
        if(--frag->tmo<=0)
        {
            ipv4_frag_free(frag);
        }
        cur = next;
    }
    return NULL;
}
void ipv4_frag_init(void)
{
    list_init(&ip_frag_list);
    mempool_init(&ip_frag_pool, frag_buff, IPV4_FRAG_MAX, sizeof(ip_frag_t));
    soft_timer_add(&ipv4_frag_timer,SOFT_TIMER_TYPE_PERIOD,IPV4_FRAG_TIMER_SCAN*1000,"IPV4_FRAG_TIMER",frag_tmo_handle,NULL,NULL);
}

void ipv4_frag_free(ip_frag_t *frag)
{
    if (!frag)
    {
        dbg_error("frag is null\r\n");
        return;
    }
    list_node_t *free_node = &frag->node;
    list_t *pkg_list = &frag->frag_list;
    while (list_count(pkg_list) > 0)
    {
        list_node_t *pkg_node = list_remove_first(pkg_list);
        pkg_t *pkg = list_node_parent(pkg_node, pkg_t, node);
        package_collect(pkg);
    }
    list_remove(&ip_frag_list, free_node);
    mempool_free_blk(&ip_frag_pool, frag);
}
ip_frag_t *ipv4_frag_alloc(void)
{
    ip_frag_t *ret = NULL;
    ip_frag_t *frag = mempool_alloc_blk(&ip_frag_pool, -1);
    if (!frag)
    {
        ip_frag_t *last_frag = list_node_parent(&ip_frag_list.last, ip_frag_t, node);
        // 把数据包都释放掉
        list_t *pkg_list = &last_frag->frag_list;
        while (list_count(pkg_list) > 0)
        {
            list_node_t *pkg_node = list_remove_first(pkg_list);
            pkg_t *pkg = list_node_parent(pkg_node, pkg_t, node);
            package_collect(pkg);
        }
        ret = last_frag;
        list_remove(&ip_frag_list, &ret->node);
    }
    else
    {
        memset(frag, 0, sizeof(ip_frag_t));
        ret = frag;
    }
    // 把刚分配的frag结构插入首部
    frag->tmo = IPV4_FRAG_TMO/IPV4_FRAG_TIMER_SCAN;
    list_insert_first(&ip_frag_list, &ret->node);
    return ret;
}
ip_frag_t *ipv4_frag_find(ipaddr_t *ip, uint16_t id)
{
    ip_frag_t *ret = NULL;
    list_t *list = &ip_frag_list;
    list_node_t *cur = list->first;
    while (cur)
    {
        ip_frag_t *frag = list_node_parent(cur, ip_frag_t, node);
        if (frag->ip.q_addr == ip->q_addr && id == frag->id)
        {
            ret = frag;
            list_remove(&ip_frag_list, &frag->node);
            list_insert_first(&ip_frag_list, &frag->node);
            break;
        }
        cur = cur->next;
    }
    return ret;
}
void ipv4_frag_print(ip_frag_t *frag)
{
    char ipbuf[20] = {0};
    ipaddr_n2s(&frag->ip, ipbuf, 20);
    dbg_info("the frag ip is:%s\r\n", ipbuf);
    dbg_info("the frag id is%d\r\n", frag->id);
    dbg_info("the pkg list like follow:\r\n");
    list_t *list = &frag->frag_list;
    list_node_t *cur = list->first;
    int count = 0;
    while (cur)
    {
        pkg_t *pkg = list_node_parent(cur, pkg_t, node);
        ipv4_header_t *head = package_data(pkg, sizeof(ipv4_header_t), 0);
        ipv4_head_parse_t parse;
        parse_ipv4_header(head, &parse);
        dbg_info("..................................\r\n");
        dbg_info("pkg_%d:\r\n", count);
        dbg_info("pkg_data_len:%d\r\n", parse.total_len - parse.head_len);
        dbg_info("flags_3,More fragment:%x\r\n", (parse.flags & 0x01) ? 1 : 0);
        dbg_info("frag_offset:%d\r\n", parse.frag_offset);
        dbg_info("..................................\r\n");
        cur = cur->next;
        count++;
    }
}
void ipv4_frag_list_print(void)
{
#ifdef DBG_IPV4_FRAG_PRINT
    list_t *frag_list = &ip_frag_list;
    list_node_t *cur_list = frag_list->first;
    int count = 0;
    while (cur_list)
    {
        dbg_info("print frag list%d++++++++++++\r\n", count);
        ip_frag_t *frag = list_node_parent(cur_list, ip_frag_t, node);
        ipv4_frag_print(frag);
        cur_list = cur_list->next;
        count++;
    }
#endif
}
int ipv4_frag_insert_pkg(ip_frag_t *frag, pkg_t *pkg, ipv4_head_parse_t *parse)
{
    if (!frag || !pkg || !parse)
    {
        return -1;
    }
    list_t *pkg_list = &frag->frag_list;
    list_node_t *cur = pkg_list->first;
    while (cur)
    {
        pkg_t *cur_pkg = list_node_parent(cur, pkg_t, node);
        ipv4_header_t *ip_head = package_data(cur_pkg, sizeof(ipv4_header_t), 0);
        int frag_flags_and_offset_host = 0;
        _ntohs(ip_head->frag_flags_and_offset, &frag_flags_and_offset_host);
        uint16_t offset = (frag_flags_and_offset_host & 0x1FFF) * 8;
        // 安顺序插入
        if (parse->frag_offset < offset)
        {
            list_insert_front(pkg_list, &cur_pkg->node, &pkg->node);
            break;
        }
        else if (parse->frag_offset == offset)
        {
            break;
        }
        cur = cur->next;
    }
    if (!cur)
    {
        list_insert_last(pkg_list, &pkg->node);
    }
    return 0;
}
int ipv4_frag_is_all_arrived(ip_frag_t *frag)
{
    list_t *pkg_list = &frag->frag_list;
    list_node_t *cur_pkg_node = pkg_list->first;
    if (list_count(pkg_list) <= 1)
    {
        return 0; // 就一个分片，肯定不全
    }
    //
    pkg_t *first_pkg = list_node_parent(cur_pkg_node, pkg_t, node);
    ipv4_header_t *first_ip_head = package_data(first_pkg, sizeof(ipv4_header_t), 0);
    ipv4_head_parse_t first_parse;
    parse_ipv4_header(first_ip_head, &first_parse);
    if (first_parse.frag_offset != 0 || !(first_parse.flags & 0x1))
    {
        return 0; // 第一个分片的偏移量不是0，肯定不全
    }
    while (cur_pkg_node)
    {
        pkg_t *cur_pkg = list_node_parent(cur_pkg_node, pkg_t, node);
        ipv4_header_t *ip_head = package_data(cur_pkg, sizeof(ipv4_header_t), 0);
        uint16_t frag_flags_and_offset_host = 0, frag_offset = 0, total_len = 0;
        uint8_t flags = 0, head_len = 0;
        // 将网络字节序转换为主机字节序
        _ntohs(ip_head->frag_flags_and_offset, &frag_flags_and_offset_host);
        // 提取 flags 和 frag_offset
        flags = (frag_flags_and_offset_host >> 13) & 0x07;       // 高3位为 flags
        frag_offset = (frag_flags_and_offset_host & 0x1FFF) * 8; // 低13位为 frag_offset
        head_len = (ip_head->version_and_ihl & 0x0F) * 4;
        _ntohs(ip_head->total_len, &total_len);
        uint16_t data_len = total_len - head_len;
        list_node_t *next = NULL;
        if (flags & 0x1)
        {
            // MF==1,应该有下一个分片
            next = cur_pkg_node->next;
            if (!next)
            {
                return 0; // 分片没有全部到达
            }
            else
            {
                pkg_t *next_pkg = list_node_parent(next, pkg_t, node);
                ipv4_header_t *next_ip_head = package_data(next_pkg, sizeof(ipv4_header_t), 0);
                uint16_t next_flags_and_offset = 0, next_offset = 0;
                _ntohs(next_ip_head->frag_flags_and_offset, &next_flags_and_offset);
                next_offset = (next_flags_and_offset & 0x1FFF) * 8; // 低13位为 frag_offset
                if (next_offset != data_len + frag_offset)
                {
                    return 0; // 分片没有全部到达
                }
                else
                {
                    cur_pkg_node = next;
                }
            }
        }
        else
        {
            return 1;
        }
    }
    return 1;
}
pkg_t *ipv4_frag_join_pkg(ip_frag_t *frag)
{
    pkg_t *to, *from;
    list_t *pkg_list = &frag->frag_list;
    list_node_t *first_pkg_node = pkg_list->first;
    pkg_t *first_pkg = list_node_parent(first_pkg_node, pkg_t, node);

    list_remove(pkg_list, first_pkg_node);
    to = first_pkg;

    first_pkg_node = pkg_list->first;
    while (first_pkg_node)
    {
        list_node_t *next_pkg_node = first_pkg_node->next;
        from = list_node_parent(first_pkg_node, pkg_t, node);
        package_shrank_front(from, sizeof(ipv4_header_t));
        list_remove(pkg_list, first_pkg_node);
        package_join(from, to);
        first_pkg_node = next_pkg_node;
    }
    return to;
}
void ipv4_init(void)
{
    ipv4_frag_init();
}