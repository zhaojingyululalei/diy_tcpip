#include "msgQ.h"
#include "mmpool.h"
#include "pkg_workbench.h"

/*消息队列结构*/
static msgQ_t workbench;
static void* workbench_buf[WORKBENCH_STUFF_CAPCITY];

/*内存池，用于管理wb_stuff_t结构的内存分配*/
static mempool_t wb_stuff_pool;
static wb_stuff_t wb_stuff_buf[WORKBENCH_STUFF_CAPCITY*(sizeof(wb_stuff_t)+sizeof(list_node_t))];

void workbench_init(void)
{
    msgQ_init(&workbench,workbench_buf,WORKBENCH_STUFF_CAPCITY);
    mempool_init(&wb_stuff_pool,&wb_stuff_buf,WORKBENCH_STUFF_CAPCITY,sizeof(wb_stuff_t));
}

pkg_t* workbench_get_stuff(void)
{
    pkg_t* ret = NULL;
    wb_stuff_t* stuff = (wb_stuff_t*)msgQ_dequeue(&workbench,0);//阻塞等
    ret = stuff->package;
    mempool_free_blk(&wb_stuff_pool,stuff);//stuff放回内存池
    return ret;

}

void workbench_put_stuff(pkg_t* package)
{
    wb_stuff_t* stuff = (wb_stuff_t*)mempool_alloc_blk(&wb_stuff_pool,-1);//非阻塞
    if(!stuff)
    {
        dbg_warning("stuff pool memory out\r\n");
        return;
    }
    stuff->package = package;
    msgQ_enqueue(&workbench,stuff,-1);
    return;
}


