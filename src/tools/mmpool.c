#include "mmpool.h"

// 初始化内存池
int mempool_init(mempool_t *mempool, uint8_t *bf, int block_limit, int block_size)
{
    if (!mempool || block_limit <= 0)
        return -1;

    mempool->buffer = bf;
    if (!mempool->buffer)
        return -1;

    list_init(&mempool->free_list);
    lock_init(&mempool->locker);
    semaphore_init(&mempool->block, block_limit);

    uint8_t *start = mempool->buffer;
    for (int i = 0; i < block_limit; i++, start += (block_size+sizeof(list_node_t)))
    {
        list_node_t *node = (list_node_t *)start;
        list_insert_last(&mempool->free_list, node);
    }

    return 0;
}

// 获取空闲块计数
uint32_t mempool_freeblk_cnt(mempool_t *mempool)
{
    uint32_t count;
    lock(&mempool->locker);
    count = mempool->free_list.count;
    unlock(&mempool->locker);
    return count;
}

/* 毫秒 */
void* mempool_alloc_blk(mempool_t *mempool, int timeout)
{

    void *ret;
    if (mempool_freeblk_cnt(mempool) > 0)
    {
        lock(&mempool->locker);
        list_node_t *fnode = list_remove_first(&mempool->free_list);
        ret = (void *)(fnode + 1);
        unlock(&mempool->locker);
        return ret;
    }
    else
    { // 暂时没有空闲内存块
        if (timeout < 0)
        {
            return NULL; // 不用等待，直接返回
        }
        else
        {
            if (time_wait(&mempool->block, timeout) == 0)
            {
                lock(&mempool->locker);
                list_node_t *fnode = list_remove_first(&mempool->free_list);
                ret = (void *)(fnode + 1);
                unlock(&mempool->locker);
                return ret;
            }
        }
    }

    return NULL;
}

// 释放内存块
int mempool_free_blk(mempool_t *mempool, void *block)
{
    if (!block)
    {
        return 0;
    }

    list_node_t* node = (list_node_t*)block-1;
    lock(&mempool->locker);
    list_insert_last(&mempool->free_list, node);
    unlock(&mempool->locker);

    post(&mempool->block);

    return 0;
}

// 销毁内存池
void mempool_destroy(mempool_t *mempool)
{
    if (!mempool)
        return;

    list_destory(&mempool->free_list);
    lock_destory(&mempool->locker);
    semaphore_destory(&mempool->block);
}