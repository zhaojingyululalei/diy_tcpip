
#ifndef __NET_CFG_H
#define __NET_CFG_H
#define IPV4_ADDR_SIZE 32
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

//#define DBG_THREAD_PRINT
#define DBG_EHTER_PRINT
#define DBG_SOFT_TIMER_PRINT


#endif