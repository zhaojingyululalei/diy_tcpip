#include "loop.h"
#include "netif.h"
#include "debug.h"

static const netif_ops_t loop_ops={
    .open = loop_open,
    .close = loop_close,
    .receive = loop_receive,
    .send = loop_send
};

netif_t* loop_init(void)
{
    netif_info_t *loop_info= (netif_info_t *)malloc(sizeof(netif_info_t));
    memset(loop_info,0,sizeof(netif_info_t));
    ipaddr_s2n(NETIF_LOOP_IPADDR,&loop_info->ipaddr);
    ipaddr_s2n(NETIF_LOOP_MASK,&loop_info->mask);
    strncpy(loop_info->name,"lo",2);
    loop_info->type = NETIF_TYPE_LOOP;


    netif_t* ret = netif_virtual_register(loop_info,&loop_ops,NULL);
    if(ret == NULL)
    {
        free(loop_info);
        dbg_error("loop init fail\r\n");
        return NULL;
    }
    
    
    return ret;
}


int loop_open(netif_t* netif,void* ex_data)
{
    int ret;
    ret = netif_open(netif,ex_data);
    if(ret< 0)
    {
        return ret;
    }

    return 0;

}
int loop_close(netif_t* netif)
{
    return 0;
}

int loop_send(netif_t* netif,const uint8_t* buf,int len)
{
    return 0;
}

int loop_receive(netif_t* netif,uint8_t* buf,int len)
{
    return 0;
}
