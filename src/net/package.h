#ifndef __PACKAGE_H
#define __PACKAGE_H
#include "list.h"
#include "debug.h"

#define PKG_DATA_BLK_SIZE   128
#define PKG_LIMIT   512
#define PKG_DATA_BLK_LIMIT  2048

typedef struct _pkg_dblk_t{
    list_node_t node;
    int size;
    int offset; //数据块起始地址偏移
    
    uint8_t data[PKG_DATA_BLK_SIZE];
}pkg_dblk_t;

typedef struct _pkg_t{
    int total;
    list_t pkgdb_list;
    list_node_t node;   //有些数据包是兄弟包，有关联的

    int pos; //指向下一个空白位置
    pkg_dblk_t* curblk;
    int inner_offset;//当前数据块的内部偏移
}pkg_t; 


pkg_dblk_t* package_get_first_datablk(pkg_t* package);
pkg_dblk_t* package_get_last_datablk(pkg_t* package);
pkg_dblk_t* package_get_next_datablk(pkg_dblk_t* cur_blk);
pkg_dblk_t *package_get_pre_datablk(pkg_dblk_t *cur_blk);
int package_remove_one_blk(pkg_t* package,pkg_dblk_t* delblk);





int package_integrate_two_continue_blk(pkg_t *package, pkg_dblk_t *blk);

//user
int package_show_pool_info(void);
int package_show_info(pkg_t *package);
int package_pool_init(void);
int package_pool_destory(void);
//报数据包头都整合到一个blk里，传入所有包头大小总和，不能超过128
int package_integrate_header(pkg_t* package,int all_hsize);
int package_join(pkg_t* from,pkg_t* to);
//返回写入长度
int package_write(pkg_t *package, const uint8_t *buf, int len);
int package_read(pkg_t *package, uint8_t *buf, int len);
int package_write_pos(pkg_t *package, const uint8_t *buf, int len,int postion);
int package_read_pos(pkg_t *package, uint8_t *buf, int len,int position);
int package_lseek(pkg_t *package, int offset);
int package_memset(pkg_t *package, int offset, uint8_t value, int len);
int package_memcpy(pkg_t *dest_pkg, int dest_offset, pkg_t *src_pkg, int src_offset, int len);
int package_copy(pkg_t* dest_pkg,pkg_t* src_pkg);
void package_print(pkg_t* pkg);
uint8_t *package_data(pkg_t *pkg,int len,int position);
//创建包并直接写入数据
pkg_t*  package_create(uint8_t* data_buf,int len);
//创建空白包，稍后写入数据
pkg_t *package_alloc(int size);
int package_collect(pkg_t *package);
//添加包头，空白数据，对齐式的开辟空间
int package_add_headspace(pkg_t* package,int h_size);
//添加数据包头，对齐式的开辟空间
int package_add_header(pkg_t* package,uint8_t* head_buf,int head_len);

int package_expand_last(pkg_t* package,int ex_size);
//前面的空间不够ex_size,紧贴前面的，然后再开辟blk
int package_expand_front(pkg_t* package,int ex_size);
//前面的空间不够，会直接再开辟一个新blk
int package_expand_front_align(pkg_t* package,int ex_size);
int package_shrank_front(pkg_t* package,int sh_size);
int package_shrank_last(pkg_t* package,int sh_size);

static inline int package_get_total(pkg_t* package)
{
    return package->total;
}
static inline int package_get_cur_pos(pkg_t* package)
{
    return package->pos;
}
uint16_t package_checksum16(pkg_t* pkg, int off,int size, uint32_t pre_sum, int complement) ;

#endif