
#ifndef __NET_CFG_H
#define __NET_CFG_H
#define IPV4_ADDR_SIZE 32
#define MAC_ARRAY_LEN 6


/*need config*/
//IP地址长度
#define IP_ADDR_SIZE IPV4_ADDR_SIZE

//网卡缓存能力，输入输出队列能缓存多少数据包
#define NETIF_INQ_BUF_MAX      50
#define NETIF_OUTQ_BUF_MAX     50

//工作台容量
#define WORKBENCH_STUFF_CAPCITY 50
#define THREADPOOL_THREADS_NR   20

#endif