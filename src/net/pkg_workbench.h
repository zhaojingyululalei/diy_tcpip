#ifndef __NET_WORKBENCH_H
#define __NET_WORKBENCH_H
#include "package.h"
#include "net_cfg.h"
#include "netif.h"
typedef struct _wb_stuff_t
{
    pkg_t* package;
    netif_t* netif;
}wb_stuff_t;

void workbench_init(void);
wb_stuff_t* workbench_get_stuff(void);
void workbench_put_stuff(pkg_t* package,netif_t* netif);
int workbench_collect_stuff(wb_stuff_t* stuff);
#endif

