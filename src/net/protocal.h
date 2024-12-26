#ifndef __PROTOCAL_H
#define __PROTOCAL_H

typedef enum _protocal_type_t
{
    PROTOCAL_TYPE_ARP = 0x0806,
    PROTOCAL_TYPE_IPV4 = 0x0800,
    PROTOCAL_TYPE_ICMPV4 = 0x1,
    PROTOCAL_TYPE_UDP = 0x11,
    PROTOCAL_TYPE_TCP = 0x6,
}protocal_type_t;

#endif