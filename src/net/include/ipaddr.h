#ifndef __IPADDR_H
#define __IPADDR_H

#include <stdint.h>
#include "net_cfg.h"
#include "net_err.h"
/**
 * @brief IP地址
 */
typedef struct _ipaddr_t {
    enum {
        IPADDR_V4,
    }type;              // 地址类型

    union {
        uint32_t q_addr;                        // 32位整体描述
        uint8_t a_addr[IPV4_ADDR_SIZE];        // 数组描述
    };
}ipaddr_t;

net_err_t parse_ipaddr(const char *ip_str, ipaddr_t *ip_addr);

#endif
