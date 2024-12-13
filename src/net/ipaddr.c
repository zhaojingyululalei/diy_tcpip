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

int ipaddr_n2s(const ipaddr_t *ip_addr, char *ip_str, size_t str_len) {
    if (!ip_addr || !ip_str || str_len < 16) { 
        return -1; 
    }

    if (ip_addr->type != IPADDR_V4) {
        return -1; 
    }

    // Format the IP address as a string
    int ret = snprintf(ip_str, str_len, "%d.%d.%d.%d",
                       ip_addr->a_addr[3], ip_addr->a_addr[2],
                       ip_addr->a_addr[1], ip_addr->a_addr[0]);
    
    // Check if snprintf was successful
    if (ret < 0 || (size_t)ret >= str_len) {
        return -1; 
    }

    return 0; 
}
/*主机小端对齐，网络大端对齐(低字节在高地址)*/
uint8_t* h2n(const uint8_t* arr,int len)
{
    uint8_t* ret = malloc(len);
    for (int i = 0; i < len; i++)
    {
        ret[i] = arr[len-1-i];
    }
    return ret;
    
}

uint8_t* n2h(const uint8_t* arr,int len)
{
    uint8_t* ret = malloc(len);
    for (int i = 0; i < len; i++)
    {
        ret[i] = arr[len-1-i];
    }
    return ret;
    
}

