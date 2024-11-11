#ifndef __MEMPOOL_H
#define __MEMPOOL_H
#include "thread.h"
#include "list.h"


/*config*/
#define MEM_BLK_SIZE    512
#define MEM_BLK_LIMIT   1024

typedef struct _mempool_t {
    list_t free_list;
    lock_t locker;
    semaphore_t block; // 用于控制内存块分配的信号量
    uint8_t *buffer; // 动态分配的内存块数组缓冲区
} mempool_t;




net_err_t mempool_init(mempool_t *mempool, uint8_t *bf, int block_limit, int block_size);
uint32_t mempool_freeblk_cnt(mempool_t *mempool);
void *mempool_alloc_blk(mempool_t *mempool, int timeout);
net_err_t mempool_free_blk(mempool_t *mempool, void *block);
void mempool_destroy(mempool_t *mempool);





#endif
