#include "include/exmsg.h"
#include "include/mempool.h"
#include "include/msg_queue.h"
#include "list.h"

static exmsg_data_t* msg_tbl[MSGQ_TBL_MAX_LIMIT+1];
static uint8_t msg_buf[MEMPOLL_CTL_MAX_LIMIT*(sizeof(list_node_t)+sizeof(exmsg_data_t))];

static msgQ_t msg_queue;
static mempool_t msg_pool;
int testid =0;

net_err_t exmsg_init(void)
{
    net_err_t ret;
    ret = msgQ_init(&msg_queue,msg_tbl,MSGQ_TBL_MAX_LIMIT);
    CHECK_NET_ERR(ret);
    ret = mempool_init(&msg_pool,(uint8_t*)msg_buf,MEMPOLL_CTL_MAX_LIMIT,sizeof(exmsg_data_t));
    CHECK_NET_ERR(ret);

    dbg_info("msg queue && msg mempool init successful\n");
    return NET_ERR_OK;
}

/*从网卡取消息，放到消息队列*/
net_err_t exmsg_netif_to_msgq(void)
{
    net_err_t ret;
    //这里以后可能用中断来放入到消息队列中，因此设置为立即返回，不用等。
    exmsg_data_t* data = mempool_alloc_blk(&msg_pool,-1);
    CHECK_NET_NULL(data);

    data->testid = testid++;
    dbg_info("netif put a message in msgQ\n");
    ret = msgQ_enqueue(&msg_queue,data,-1);
    CHECK_NET_ERR(ret);
    return NET_ERR_OK;
}

exmsg_data_t* exmsg_getmsg_from_msgq(void)
{
    //工作线程会调用这个函数，工作线程也不干别的。因此，没有消息就一直等。
    return  msgQ_dequeue(&msg_queue,0);
}