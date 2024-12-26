
#ifndef __NET_CFG_H
#define __NET_CFG_H
#define IPV4_ADDR_SIZE 32
#define IPV4_ADDR_ARR_LEN   (IPV4_ADDR_SIZE/8)
#define MAC_ADDR_ARR_LEN    6
#define MAC_ARRAY_LEN 6


/*need config*/
//IP地址长度
#define IP_ADDR_SIZE IPV4_ADDR_SIZE

//网卡缓存能力，输入输出队列能缓存多少数据包
#define NETIF_INQ_BUF_MAX      50
#define NETIF_OUTQ_BUF_MAX     50

//工作台容量
#define WORKBENCH_STUFF_CAPCITY 50
//整个网络线程池，最大线程数量
#define THREADPOOL_THREADS_NR   20
//最大网卡数量
#define NETIF_MAX_NR    20

#define NETIF_LOOP_IPADDR   "127.0.0.1"
#define NETIF_LOOP_MASK     "255.255.255.0"

#define NETIF_PH_IPADDR     "192.168.169.10"
#define NETIF_PH_MASK       "255.255.255.0"
#define NETIF_PH_GATEWAY    "192.168.128.2"

#define BROADCAST_MAC_ADDR  "ff:ff:ff:ff:ff:ff"
#define EMPTY_MAC_ADDR      "00:00:00:00:00:00"

//#define DBG_THREAD_PRINT
#define DBG_EHTER_PRINT
#define DBG_SOFT_TIMER_PRINT
#define DBG_ARP_PRITN
#define DBG_IPV4_PRINT


//arp配置
//arp表项最大数量
#define ARP_ENTRY_MAX_SIZE  64  
//arp每个表项做多缓存多少数据包
#define ARP_ENTRY_PKG_CACHE_MAX_SIZE    5
#define ARP_ENTRY_TMO_STABLE    (20*60)    //表项更新周期
#define ARP_ENTRY_TMO_RESOLVING 3    //发起更新命令后，限时3s完成
#define ARP_ENTRY_RETRY 5           //给5次机会，还没完成就删除表项

#endif