#ifndef __NET_WORKBENCH_H
#define __NET_WORKBENCH_H
#include "package.h"
#include "net_cfg.h"

typedef struct _wb_stuff_t
{
    pkg_t* package;
}wb_stuff_t;

void workbench_init(void);
pkg_t* workbench_get_stuff(void);
void workbench_put_stuff(pkg_t* package);

#endif

