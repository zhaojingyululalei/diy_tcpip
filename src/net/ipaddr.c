#include "ipaddr.h"
int ipaddr_s2n(const char *ip_str, ipaddr_t *ip_addr)
{
    if (!ip_str || !ip_addr) {
        return -1; 
    }
    ip_addr->type = IPADDR_V4;
    uint8_t bytes[IP_ARRAY_LEN] = {0};
    int byte = 0, index = 0;

    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9') {
            byte = byte * 10 + (*ip_str - '0'); 
            if (byte > 255) {
                return -1; 
            }
        } else if (*ip_str == '.') {
            if (index >= IP_ARRAY_LEN - 1) {
                return -1; 
            }
            bytes[index++] = byte; 
            byte = 0;              
        } else {
            return -1; 
        }
        ip_str++;
    }

    if (index != IP_ARRAY_LEN - 1) {
        return -1; 
    }
    bytes[index] = byte; 

    for (int i = 0; i < IP_ARRAY_LEN; ++i) {
        ip_addr->a_addr[i] = bytes[i];
    }


    ip_addr->q_addr = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

    return 0; 
}

