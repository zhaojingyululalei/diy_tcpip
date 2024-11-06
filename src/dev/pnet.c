
#include "pnet.h"

/**
 * 根据ip地址查找本地网络接口列表，找到相应的名称
 */
int pcap_find_device(const char *ip, char *name_buf)
{
    struct in_addr dest_ip;

    inet_pton(AF_INET, ip, &dest_ip);

    // 获取所有的接口列表
    char err_buf[PCAP_ERRBUF_SIZE];
    pcap_if_t *pcap_if_list = NULL;
    int err = pcap_findalldevs(&pcap_if_list, err_buf);
    if (err < 0)
    {
        pcap_freealldevs(pcap_if_list);
        return -1;
    }

    // 遍历列表
    pcap_if_t *item;
    for (item = pcap_if_list; item != NULL; item = item->next)
    {
        if (item->addresses == NULL)
        {
            continue;
        }

        // 查找地址
        for (struct pcap_addr *pcap_addr = item->addresses; pcap_addr != NULL; pcap_addr = pcap_addr->next)
        {
            // 检查ipv4地址类型
            struct sockaddr *sock_addr = pcap_addr->addr;
            if (sock_addr->sa_family != AF_INET)
            {
                continue;
            }

            // 地址相同则返回
            struct sockaddr_in *curr_addr = ((struct sockaddr_in *)sock_addr);
            if (curr_addr->sin_addr.s_addr == dest_ip.s_addr)
            {
                strcpy(name_buf, item->name);
                pcap_freealldevs(pcap_if_list);
                return 0;
            }
        }
    }

    pcap_freealldevs(pcap_if_list);
    return -1;
}

/*
 * 显示所有的网络接口列表，在出错时被调用
 */
int pcap_show_list(void) {
    char err_buf[PCAP_ERRBUF_SIZE];
    pcap_if_t* pcapif_list = NULL;
    int count = 0;

    // 查找所有的网络接口
    int err = pcap_findalldevs(&pcapif_list, err_buf);
    if (err < 0) {
        fprintf(stderr, "scan net card failed: %s\n", err_buf);
        pcap_freealldevs(pcapif_list);
        return -1;
    }

    printf("net card list: \n");

    // 遍历所有的可用接口，输出其信息
    for (pcap_if_t* item = pcapif_list; item != NULL; item = item->next) {
        if (item->addresses == NULL) {
            continue;
        }

        // 显示ipv4地址
        for (struct pcap_addr* pcap_addr = item->addresses; pcap_addr != NULL; pcap_addr = pcap_addr->next) {
            char str[INET_ADDRSTRLEN];
            struct sockaddr_in* ip_addr;

            struct sockaddr* sockaddr = pcap_addr->addr;
            if (sockaddr->sa_family != AF_INET) {
                continue;
            }

            ip_addr = (struct sockaddr_in*)sockaddr;
            char * name = item->description;
            if (name == NULL) {
                name = item->name;
            }
            printf("%d: IP:%s name: %s,\n",count++,inet_ntop(AF_INET, &ip_addr->sin_addr, str, sizeof(str)),name?name:"unkown");
            break;
        }
    }

    pcap_freealldevs(pcapif_list);

    if ((pcapif_list == NULL) || (count == 0)) {
        fprintf(stderr, "error: no net card!\n");
        return -1;
    }

    printf("no net card found, check system configuration\n");
    return 0;
}

/**
 * 打开pcap设备接口
 */
pcap_t * pcap_device_open(const char* ip, const uint8_t* mac_addr) {
    

    // 利用上层传来的ip地址，
    char name_buf[256];
    if (pcap_find_device(ip, name_buf) < 0) {
        fprintf(stderr, "pcap find error: no net card has ip: %s. \n", ip);
        pcap_show_list();
        return (pcap_t*)0;
    }

    // 根据名称获取ip地址、掩码等
    char err_buf[PCAP_ERRBUF_SIZE];
    bpf_u_int32 mask;
    bpf_u_int32 net;
    if (pcap_lookupnet(name_buf, &net, &mask, err_buf) == -1) {
        printf("pcap_lookupnet error: no net card: %s\n", name_buf);
        net = 0;
        mask = 0;
    }

    // 打开设备
    pcap_t * pcap = pcap_create(name_buf, err_buf);
    if (pcap == NULL) {
        fprintf(stderr, "pcap_create: create pcap failed %s\n net card name: %s\n", err_buf, name_buf);
        fprintf(stderr, "Use the following:\n");
        pcap_show_list();
        return (pcap_t*)0;
    }

    if (pcap_set_snaplen(pcap, 65536) != 0) {
        fprintf(stderr, "pcap_open: set none block failed: %s\n", pcap_geterr(pcap));
        return (pcap_t*)0;
    }

    if (pcap_set_promisc(pcap, 1) != 0) {
        fprintf(stderr, "pcap_open: set none block failed: %s\n", pcap_geterr(pcap));
        return (pcap_t*)0;
    }

    if (pcap_set_timeout(pcap, 0) != 0) {
        fprintf(stderr, "pcap_open: set none block failed: %s\n", pcap_geterr(pcap));
        return (pcap_t*)0;
    }

    // 非阻塞模式读取，程序中使用查询的方式读
    if (pcap_set_immediate_mode(pcap, 1) != 0) {
        fprintf(stderr, "pcap_open: set im block failed: %s\n", pcap_geterr(pcap));
        return (pcap_t*)0;
    }

    if (pcap_activate(pcap) != 0) {
        fprintf(stderr, "pcap_open: active failed: %s\n", pcap_geterr(pcap));
        return (pcap_t*)0;
    }

    if (pcap_setnonblock(pcap, 0, err_buf) != 0) {
        fprintf(stderr, "pcap_open: set none block failed: %s\n", pcap_geterr(pcap));
        return (pcap_t*)0;
    }

    // 只捕获发往本接口与广播的数据帧。相当于只处理发往这张网卡的包
    char filter_exp[256];
    struct bpf_program fp;
    sprintf(filter_exp,
        "(ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether broadcast)",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    if (pcap_compile(pcap, &fp, filter_exp, 0, net) == -1) {
        printf("pcap_open: couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(pcap));
        return (pcap_t*)0;
    }
    if (pcap_setfilter(pcap, &fp) == -1) {
        printf("pcap_open: couldn't install filter %s: %s\n", filter_exp, pcap_geterr(pcap));
        return (pcap_t*)0;
    }
    return pcap;
}
