#include "include/ipaddr.h"
#include <stdio.h>

net_err_t parse_ipaddr(const char *ip_str, ipaddr_t *ip_addr) {
    if (!ip_str || !ip_addr) {
        return NET_ERR_SYS; 
    }
    ip_addr->type = IPADDR_V4;
    uint8_t bytes[IPV4_ADDR_SIZE] = {0};
    int byte = 0, index = 0;

    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9') {
            byte = byte * 10 + (*ip_str - '0'); 
            if (byte > 255) {
                return NET_ERR_SYS; 
            }
        } else if (*ip_str == '.') {
            if (index >= IPV4_ADDR_SIZE - 1) {
                return NET_ERR_SYS; 
            }
            bytes[index++] = byte; 
            byte = 0;              
        } else {
            return NET_ERR_SYS; 
        }
        ip_str++;
    }

    if (index != IPV4_ADDR_SIZE - 1) {
        return NET_ERR_SYS; 
    }
    bytes[index] = byte; 

    for (int i = 0; i < IPV4_ADDR_SIZE; ++i) {
        ip_addr->a_addr[i] = bytes[i];
    }


    ip_addr->q_addr = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

    return NET_ERR_OK; 
}