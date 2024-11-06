#ifndef __PNET_H
#define __PNET_H

#include <pcap/pcap.h>

int pcap_find_device(const char *ip, char *name_buf);
int pcap_show_list(void) ;
pcap_t * pcap_device_open(const char* ip, const uint8_t* mac_addr);
#endif
