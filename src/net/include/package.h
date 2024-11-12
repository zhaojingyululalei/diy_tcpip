#ifndef __PACKAGE_H
#define __PACKAGE_H
#include "list.h"
#include "debug.h"
#include "net_err.h"

#define PKG_DATA_BLK_SIZE   128
#define PKG_LIMIT   512
#define PKG_DATA_BLK_LIMIT  2048

typedef struct _pkg_dblk_t{
    list_node_t node;
    int size;
    int offset;
    uint8_t data[PKG_DATA_BLK_SIZE];
}pkg_dblk_t;

typedef struct _pkg_t{
    int total;
    list_t pkgdb_list;
    list_node_t node;   //有些数据包是兄弟包，有关联的
}pkg_t; 


net_err_t package_pool_init(void);
net_err_t package_pool_destory(void);
net_err_t package_collect(pkg_t *package);
pkg_t* package_create(int size);
void package_destory(pkg_t *package);
void package_datablk_destory(pkg_dblk_t *blk);
#endif