#ifndef __NET_DRIVE_H
#define __NET_DRIVE_H

#include <pcap/pcap.h>
/*虚拟机新添加了一个网卡,only-host模式， 在netplan中自己进行了配置，命名为ens37*/
int pcap_find_device(const char *ip, char *name_buf);
int pcap_show_list(void) ;

pcap_t * pcap_device_open(const char* ip, const uint8_t* mac_addr);
int pcap_recv_pkg(pcap_t * handler,  const uint8_t **pkg_data);
int pcap_send_pkg(pcap_t* handler,uint8_t* buffer,int size);

#endif