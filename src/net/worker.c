#include "include/worker.h"
#include "thread.h"
#include "msg_queue.h"
#include "include/mempool.h"
#include "include/exmsg.h"



static DEFINE_THREAD_FUNC(worker)
{
    printf("worker is running...\n");
    exmsg_data_t* message;
    while (1)
    {
        message = exmsg_getmsg_from_msgq();
        printf("woker recv a message,id is %d\n",message->testid);
        sleep(1);
    }
    
    
    
    return NULL;
}


net_err_t worker_init(void)
{
    net_err_t ret;
    ret =  exmsg_init();
    CHECK_NET_ERR(ret);
    return NET_ERR_OK;
}
net_err_t worker_start(void)
{
    thread_t* t;
    t = thread_create(worker,NULL);
    if(t==NULL)
    {
        return NET_ERR_SYS;
    }
    return NET_ERR_OK;
}


