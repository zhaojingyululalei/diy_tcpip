#include "net.h"
#include "include/worker.h"
net_err_t net_init(void)
{
    net_err_t result ;
    result = threads_init();
    CHECK_NET_ERR(result);
    result = worker_init();
    CHECK_NET_ERR(result);

    return NET_ERR_OK;
}

net_err_t net_start(void){
    worker_start();
    return NET_ERR_OK;
}