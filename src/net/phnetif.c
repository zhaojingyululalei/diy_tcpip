
#include "phnetif.h"
#include "netif.h"
static const netif_ops_t phnetif_ops = {
    .open = phnetif_open,
    .close = phnetif_close,
    .receive = phnetif_receive,
    .send = phnetif_send};

/*初始化一块物理网卡*/
netif_t *phnetif_init(void)
{

    netif_card_info_t *card_info = get_one_specified_card(1);
    phnetif_drive_data_t *ex_data = malloc(sizeof(phnetif_drive_data_t));
    memset(ex_data, 0, sizeof(phnetif_drive_data_t));
    ex_data->card_info = card_info;

    /*填充netif_info信息，这里是用户自己可用设置的*/
    netif_info_t *phnetif_info = (netif_info_t *)malloc(sizeof(netif_info_t));
    memset(phnetif_info, 0, sizeof(netif_info_t));
    ipaddr_s2n(NETIF_PH_IPADDR, &phnetif_info->ipaddr);
    ipaddr_s2n(NETIF_PH_MASK, &phnetif_info->mask);
    ipaddr_s2n(NETIF_PH_GATEWAY, &phnetif_info->gateway);
    strncpy(phnetif_info->name, "ens37", 5);

    netif_t *ret = netif_register(phnetif_info, &phnetif_ops, ex_data);
    if (ret == NULL)
    {
        free(phnetif_info);
        dbg_error("phnetif init fail\r\n");
        return NULL;
    }

    return ret;
}

int phnetif_open(netif_t *netif, void *ex_data)
{
    if (!netif)
    {
        return -1;
    }
    netif->info.type = NETIF_TYPE_ETH;
    
    phnetif_drive_data_t *data = (phnetif_drive_data_t *)ex_data;
    netif_card_info_t *card_info = data->card_info;

    
    uint8_t *mac_addr = card_info->mac;

    pcap_t *handler = pcap_device_open(card_info->ipv4, mac_addr);
    if (!handler)
    {
        dbg_error("phnetif open fail\r\n");
        return -2;
    }
    data->handler = handler;

    

    return 0;
}
int phnetif_close(netif_t *netif)
{
    phnetif_drive_data_t *exdata = (phnetif_drive_data_t *)netif->ex_data;
    pcap_close(exdata->handler);
    return 0;
}

int phnetif_send(netif_t *netif, const uint8_t *buf, int len)
{
    int ret;
#ifdef DBG_THREAD_PRINT
   dbg_info("send a package \r\n");
    for (int i = 0; i < len; ++i)
    {

       dbg_info("%x ", buf[i]);
        
    }
   dbg_info("\r\n");
#endif
    phnetif_drive_data_t *exdata = (phnetif_drive_data_t *)netif->ex_data;
    ret = pcap_send_pkg(exdata->handler, buf, len);
    return ret;
}

int phnetif_receive(netif_t *netif, const uint8_t **buf, int len)
{
    int ret;
    phnetif_drive_data_t *exdata = (phnetif_drive_data_t *)netif->ex_data;
    uint8_t *rbuf = NULL;
    ret = pcap_recv_pkg(exdata->handler, &rbuf);
    int cpy_size = ret > len ? len : ret;
    *buf = malloc(cpy_size);
    memcpy(*buf, rbuf, cpy_size);
#ifdef DBG_THREAD_PRINT
   dbg_info("receive a package,put into in_queue\r\n");
    for (int i = 0; i < cpy_size; ++i)
    {

       dbg_info("%x ", (*buf)[i]);
        
    }
   dbg_info("\r\n");
#endif
    return cpy_size;
}