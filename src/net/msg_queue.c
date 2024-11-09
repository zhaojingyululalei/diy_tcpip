#include "include/msg_queue.h"
#include "thread.h"

static DEFINE_THREAD_FUNC(msgQ_worker)
{
    printf("msgQ working.......\n");
    while (1)
    {
        sleep(1);
    }
    
    return NULL;
}
net_err_t msgQ_init(void)
{
    return NET_ERR_OK;
}

net_err_t msgQ_start(void)
{
    thread_t* t;
    t = thread_create(msgQ_worker,NULL);
    if(t==NULL)
    {
        return NET_ERR_SYS;
    }
    return NET_ERR_OK;
}
