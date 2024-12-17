#include "networker.h"
#include "pkg_workbench.h"
#include "threadpool.h"
#include "netif.h"
semaphore_t mover_sem;
// 数据包搬运工,采集网卡数据,放入工作台
DEFINE_THREAD_FUNC(mover)
{
    while (1)
    {
        wait(&mover_sem); // 阻塞等
        // 遍历netif_list
        netif_t *cur = netif_first();
        while (cur)
        {
            // 从inq获取一个数据包放入工作台
            pkg_t *pkg = netif_getpkg(&cur->in_q); // 非阻塞
            if (pkg)
            {

                workbench_put_stuff(pkg, cur);
                dbg_info("mover put a stuff on workbench\n");
                break;
            }
            cur = netif_next(cur);
        }
    }
}

pkg_t *handle(netif_t* netif,pkg_t *pkg)
{
    uint8_t buf[2] = {0x55, 0xAA};
    package_lseek(pkg, 0);
    package_write(pkg, buf, 2);
    //如果数据链路层操作存在，将数据包交给数据链路层处理
    if(netif->link_ops)
    {
        netif->link_ops->in(netif,pkg);
    }
    return NULL;
}
// 从工作台拿东西进行处理
DEFINE_THREAD_FUNC(worker)
{
    while (1)
    {
        uint8_t *buf = NULL;
        wb_stuff_t *stuff = workbench_get_stuff();
        pkg_t *pkg = stuff->package;
        netif_t *netif = stuff->netif;
        workbench_collect_stuff(stuff);
        dbg_info("worker get stuff pakcage,len = %d\n", pkg->total);
        handle(netif,pkg);
        // 向哪个网卡发，得解析包头判断，这里测试，默认向loop发
        //  netif_t* loop = netif_first();
        //  netif_putpkg(&loop->out_q,newpack);

        
        if (netif) //这个数据包来自网卡，经过处理，再由该网卡发出
        {

            netif_putpkg(&netif->out_q, pkg);
        }
        else  //该数据包来自应用程序，并非某个网卡
        {
            
        }
    }
}

#include "net.h"

void networker_start(void)
{
    semaphore_init(&mover_sem, 0);
    thread_create(&netthread_pool, worker, NULL);
    thread_create(&netthread_pool, mover, NULL);
}
