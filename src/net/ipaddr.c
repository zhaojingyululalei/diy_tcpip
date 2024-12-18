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
void h2n(const void* arr,int len,void* data)
{
    uint8_t* arrx = (uint8_t*)arr;
    uint8_t* ret = (uint8_t*)data;
    for (int i = 0; i < len; i++)
    {
        ret[i] = arrx[len-1-i];
    }
    return ;
    
}

void n2h(const void* arr,int len,void* data)
{
    uint8_t* arrx = (uint8_t*)arr;
    uint8_t* ret = (uint8_t*)data;
    for (int i = 0; i < len; i++)
    {
        ret[i] = arrx[len-1-i];
    }
    return ;
    
}



int mac_n2s(const uint8_t* mac, char* mac_str)
{
    // 用于存放一个 MAC 地址的字符串表示（格式：xx:xx:xx:xx:xx:xx）
    // 每个字节是两位十六进制数，使用冒号分隔
    for (int i = 0; i < 6; ++i) {
        // 每个字节需要转成两位十六进制字符，%02x 表示输出两位，不足补零
        sprintf(mac_str + (i * 3), "%02x", mac[i]);
        
        // 如果不是最后一个字节，插入冒号
        if (i < 5) {
            mac_str[(i * 3) + 2] = ':';
        }
    }
    mac_str[17] = '\0';  // 结束符，17是因为6个字节 * 3 + 1个结束符
    return 0;
}

int mac_s2n(uint8_t* mac, const char* mac_str)
{
    int j = 0;
    
    for (int i = 0; i < 17; i += 3) { // 每次读取3个字符，两个十六进制字符和一个冒号
        // 如果是冒号跳过
        if (mac_str[i] == ':') {
            continue;
        }

        // 将两个十六进制字符转换为一个字节
        uint8_t byte = 0;
        for (int k = 0; k < 2; ++k) {
            byte <<= 4;
            if (mac_str[i + k] >= '0' && mac_str[i + k] <= '9') {
                byte |= (mac_str[i + k] - '0');
            } else if (mac_str[i + k] >= 'a' && mac_str[i + k] <= 'f') {
                byte |= (mac_str[i + k] - 'a' + 10);
            } else if (mac_str[i + k] >= 'A' && mac_str[i + k] <= 'F') {
                byte |= (mac_str[i + k] - 'A' + 10);
            }
        }

        mac[j++] = byte; // 将字节存入数组
    }

    return 0;
}

