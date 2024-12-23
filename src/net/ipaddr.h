#ifndef __IPADDR_H
#define __IPADDR_H
#include "types.h"
#include "net_cfg.h"
#include "debug.h"
#define IP_ARRAY_LEN    (IP_ADDR_SIZE/8)
/**
 * @brief IP地址
 */
typedef struct _ipaddr_t {
    enum {
        IPADDR_V4,
    }type;              // 地址类型

    union {
        uint32_t q_addr;                        // 32位整体描述
        uint8_t a_addr[IP_ARRAY_LEN];        // 数组描述
    };
}ipaddr_t;

uint8_t * get_mac_broadcast(void);

int ipaddr_s2n(const char *ip_str, ipaddr_t *ip_addr);
int ipaddr_n2s(const ipaddr_t *ip_addr, char *ip_str, size_t str_len);

/*主机小端对齐，网络大端对齐(低字节在高地址)*/
void h2n(const void* host,int len,void* net);
void n2h(const void* net,int len,void* host);
void _htons(uint16_t host,void* net);
void _htonl(uint32_t host,void* net);
void _ntohs(uint16_t net,void* host);
void _ntohl(uint32_t net,void* host);

int mac_n2s(const uint8_t* mac, char* mac_str);
int mac_s2n(uint8_t* mac, const char* mac_str);

uint32_t ipaddr_get_host(ipaddr_t *ip,ipaddr_t *mask);
uint32_t ipaddr_get_net(ipaddr_t * ip,ipaddr_t *mask);
int is_local_boradcast(ipaddr_t * ip,ipaddr_t *mask);
int is_global_boradcast(ipaddr_t* ip);
uint8_t* get_mac_empty(void);
uint8_t * get_mac_broadcast(void);
int is_mac_empty(uint8_t* mac);
int is_mac_broadcast(uint8_t* mac);
#endif
