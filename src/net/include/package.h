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


pkg_dblk_t* package_get_first_datablk(pkg_t* package);
pkg_dblk_t* package_get_last_datablk(pkg_t* package);
pkg_dblk_t* package_get_next_datablk(pkg_dblk_t* cur_blk);
pkg_dblk_t *package_get_pre_datablk(pkg_dblk_t *cur_blk);
net_err_t package_remove_one_blk(pkg_t* package,pkg_dblk_t* delblk);
net_err_t package_expand_front(pkg_t* package,int ex_size);
net_err_t package_expand_last(pkg_t* package,int ex_size);
net_err_t package_expand_front_align(pkg_t* package,int ex_size);
net_err_t package_shrank_front(pkg_t* package,int sh_size);
net_err_t package_shrank_last(pkg_t* package,int sh_size);



net_err_t package_show_pool_info(void);
net_err_t package_show_info(pkg_t *package);
net_err_t package_pool_init(void);
net_err_t package_pool_destory(void);
net_err_t package_collect(pkg_t *package);
pkg_t *package_alloc(int size);
net_err_t package_add_headspace(pkg_t* package,int h_size);
net_err_t package_integrate_header(pkg_t* package,int all_hsize);
bool package_integrate_two_continue_blk(pkg_t *package, pkg_dblk_t *blk);
net_err_t package_join(pkg_t* from,pkg_t* to);

#endif