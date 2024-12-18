#include "ether.h"
#include "netif.h"

static int(ether_open)(struct _netif_t *netif)
{
    return 0;
}
static void(ether_close)(struct _netif_t *netif)
{
    return;
}
static int ether_pkg_is_ok(ether_header_t *header, int pkg_len)
{
    if (pkg_len > sizeof(ether_header_t) + MTU_MAX_SIZE)
    {
        dbg_warning("ether pkg len over size\r\n");
        return -1;
    }

    if (pkg_len < sizeof(ether_header_t))
    {
        dbg_warning("ether pkg len so small\r\n");
        return -2;
    }
    return 0;
}

static void ether_dbg_print_pkg(pkg_t *pkg)
{
    dbg_info("***************ether link layer handling a pkg***********\r\n");
    uint8_t data[MTU_MAX_SIZE];
    ether_header_t *header = package_data(pkg, sizeof(ether_header_t), 0);
    char destbuf[18] = {0};
    char srcbuf[18] = {0};
    mac_n2s(header->dest,destbuf);
    mac_n2s(header->src,srcbuf);
    uint16_t pro_type = 0;
    n2h(&header->protocal,2,&pro_type);//type占2字节
    dbg_info("dest_mac:%s\r\n",destbuf);
    dbg_info("src_mac:%s\r\n",srcbuf);
    dbg_info("protocal:%04x\r\n", pro_type);
    dbg_info("data:\r\n");
    int data_len = pkg->total - sizeof(ether_header_t);
    package_read_pos(pkg, data, data_len, sizeof(ether_header_t));
    for (int i = 0; i < data_len; ++i)
    {

        dbg_info("%02x ", data[i]);
    }
    dbg_info("\r\n");
    dbg_info("*********************************************\r\n");
}
static int(ether_in)(struct _netif_t *netif, pkg_t *package)
{

    ether_header_t *header = package_data(package, sizeof(ether_header_t), 0);
    if (ether_pkg_is_ok(header, package->total) < 0)
    {
        dbg_warning("ether pkg problem,can not handle\r\n");
        return -1;
    }
#ifdef DBG_EHTER_PRINT
    ether_dbg_print_pkg(package);
#endif
    return 0;
}
static int(ether_out)(struct _netif_t *netif, ipaddr_t *dest, pkg_t *package)
{
    return 0;
}
static const link_layer_t link_layer = {
    .type = NETIF_TYPE_ETH,
    .open = ether_open,
    .close = ether_close,
    .in = ether_in,
    .out = ether_out,
};
void ether_init(void)
{
    netif_register_link_layer(&link_layer);
}

