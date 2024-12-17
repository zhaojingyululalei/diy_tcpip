#include "ether.h"
#include "netif.h"

static int (ether_open)(struct _netif_t *netif)
{
    return 0;
}
static void (ether_close)(struct _netif_t *netif)
{
    return;
}
static int (ether_in)(struct _netif_t *netif, pkg_t *package)
{
    dbg_info("ether link layer handling a in package\r\n");
    return 0;
}
static int (ether_out)(struct _netif_t *netif, ipaddr_t *dest, pkg_t *package)
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