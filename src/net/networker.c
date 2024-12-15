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
            //从inq获取一个数据包放入工作台
            pkg_t *pkg = netif_getpkg(&cur->in_q); //非阻塞
            if (pkg)
            {

                workbench_put_stuff(pkg);
                dbg_info("mover put a stuff on workbench\n");
                break;
            }
            cur = netif_next(cur);
        }
    }
}

pkg_t* handle(pkg_t* pkg)
{
    char *tmp= "hello" ;
    dbg_info("handling a package\r\n");
    sleep(1);
    pkg_t* newpack = package_create(tmp,strlen(tmp));
    return newpack;
}
// 从工作台拿东西进行处理
DEFINE_THREAD_FUNC(worker)
{
    while (1)
    {
       uint8_t* buf = NULL;
        pkg_t *pkg = workbench_get_stuff();
        dbg_info("worker get stuff pakcage,len = %d\n", pkg->total);
        pkg_t* newpack = handle(pkg);
        package_collect(pkg);
        //向哪个网卡发，得判断，这里测试，默认向loop发
        netif_t* loop = netif_first();
        netif_putpkg(&loop->out_q,newpack);
        
    }
}

#include "net.h"

void networker_start(void)
{
    semaphore_init(&mover_sem, 0);
    thread_create(&netthread_pool, worker, NULL);
    thread_create(&netthread_pool, mover, NULL);

    
}
