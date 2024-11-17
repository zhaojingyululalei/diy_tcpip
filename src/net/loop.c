#include "include/loop.h"





net_err_t lo_open(lo_netif_t* lo,const char* name,void* data)
{
   
    printf("loop netif open ...\n");

    lo->netif = netif_open(name,data);
    lo->netif->type = NETIF_TYPE_LOOP;

    parse_ipaddr("127.0.0.1",&lo->netif->ipaddr);
    parse_ipaddr("255.0.0.0",&lo->netif->netmask);

    printf("loop netif open finish...\n");
    return NET_ERR_OK;
}

net_err_t lo_close(lo_netif_t* lo)
{
    
    netif_close(lo->netif);
    lo->netif  = NULL;
    return NET_ERR_OK;
}

net_err_t lo_transsimit(lo_netif_t* lo)
{
    return netif_transsimit(lo->netif);
}

net_err_t lo_active(lo_netif_t* lo)
{
    return netif_active(lo->netif);
}

net_err_t lo_deactive(lo_netif_t* lo)
{
    return netif_deactive(lo->netif);
}
net_err_t lo_set_ipaddr(lo_netif_t* lo,ipaddr_t* ip,ipaddr_t* mask,ipaddr_t* gateway)
{
    return netif_set_ipaddr(lo->netif,ip,mask,gateway);
}
net_err_t lo_set_hwaddr(lo_netif_t* lo,netif_hwaddr_t* hwaddr)
{
    return netif_set_hwaddr(lo->netif,hwaddr);
}
