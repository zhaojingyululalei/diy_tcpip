#include "net.h"
#include "include/msg_queue.h"
net_err_t net_init(void)
{
    net_err_t result ;
    result = threads_init();
    CHECK_NET_ERR(result);
    result = msgQ_init();
    CHECK_NET_ERR(result);

    return NET_ERR_OK;
}

net_err_t net_start(void){
    msgQ_start();
    return NET_ERR_OK;
}