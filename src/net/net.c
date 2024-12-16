
#include "networker.h"
#include "pkg_workbench.h"
#include "threadpool.h"
#include "loop.h"
#include "networker.h"
threadpool_t netthread_pool;

/**
 * 初始化net系统
 */
void net_system_init(void)
{
    //物理网卡检测
    pcap_drive_init();
    package_pool_init();

    workbench_init();

    threadpool_attr_t pool_attr;
    pool_attr.threads_nr = THREADPOOL_THREADS_NR;
    threadpool_init(&netthread_pool,&pool_attr);
    //netif链表
    netif_init();
    return;
    
}

/**
 * 系统所有线程开始创建并工作
 */
void net_system_start(void)
{
    
    networker_start();
}