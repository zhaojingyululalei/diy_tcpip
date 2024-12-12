#include "netif.h"

static uint8_t mac_addr_Host[6] = {0x0,0x0c,0x29,0x6e,0x06,0x0c};
uint8_t* get_mac_addr(const char* name)
{
    if(strncmp(name,"ens37",5)==0)
    {
        return mac_addr_Host;
    }
    return NULL;
}
