#ifndef __ICMPV4_H
#define __ICMPV4_H

#include "types.h"
#include "netif.h"
#include "package.h"
/**
 * @brief ICMP包类型
 */
typedef enum _icmp_type_t {
    ICMPv4_ECHO_REPLY = 0,                  // 回送响应
    ICMPv4_ECHO_REQUEST = 8,                // 回送请求
    ICMPv4_UNREACH = 3,                     // 不可达
}icmp_type_t;

/**
 * @brief ICMP包代码
 */
typedef enum _icmp_code_t {
    ICMPv4_ECHO = 0,                        // echo的响应码
    ICMPv4_UNREACH_PRO = 2,                 // 协议不可达
    ICMPv4_UNREACH_PORT = 3,                // 端口不可达
}icmp_code_t;
#pragma pack(1)
typedef struct _icmpv4_head_t {
    uint8_t type;           // 类型
    uint8_t code;			// 代码
    uint16_t checksum;	    // ICMP报文的校验和
}icmpv4_head_t;

/**
 * ICMP报文
 */
typedef struct _icmpv4_pkt_t {
    icmpv4_head_t hdr;            // 头和数据区
    union {
        uint32_t reverse;       // 保留项
    };
    
}icmpv4_pkt_t;
#pragma pack()


void icmpv4_init(void);
int icmpv4_in(ipaddr_t *src,ipaddr_t* host,pkg_t* pkg,int ipv4_head_len);
int icmpv4_out(ipaddr_t* src,ipaddr_t* dest,pkg_t* pkg);
int icmpv4_send_reply(ipaddr_t* src,ipaddr_t* dest,pkg_t* pkg);
int icmpv4_send_unreach(ipaddr_t* src,ipaddr_t* dest,pkg_t* pkg,int code);
#endif