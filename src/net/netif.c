#include "include/netif.h"
#include "thread.h"

static DEFINE_THREAD_FUNC(receiver)
{
    printf("receiver working......\n");
    while (1)
    {
        sleep(1);
    }
    
    return NULL;
}

static DEFINE_THREAD_FUNC(transimtor)
{
    printf("transimtor working......\n");
    while (1)
    {
        sleep(1);
    }
    
    return NULL;
}
net_err_t netif_open(void)
{
    thread_t* recv,*tran;
    recv = thread_create(receiver,NULL);
    tran = thread_create(transimtor,NULL);

    return NET_ERR_OK;
}