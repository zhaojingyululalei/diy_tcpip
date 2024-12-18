#ifndef __IPADDR_H
#define __IPADDR_H
#include "types.h"
#include "net_cfg.h"
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

int ipaddr_s2n(const char *ip_str, ipaddr_t *ip_addr);
int ipaddr_n2s(const ipaddr_t *ip_addr, char *ip_str, size_t str_len);

/*主机小端对齐，网络大端对齐(低字节在高地址)*/
void h2n(const void* arr,int len,void* data);
void n2h(const void* arr,int len,void* data);

int mac_n2s(const uint8_t* mac, char* mac_str);
int mac_s2n(uint8_t* mac, const char* mac_str);
#endif
