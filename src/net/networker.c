#include "networker.h"
#include "pkg_workbench.h"
#include "threadpool.h"
#include "netif.h"
#include "soft_timer.h"
#include "timer.h"
#include "ipv4.h"
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
            pkg_t *pkg = netif_getpkg(&cur->in_q, -1); // 非阻塞
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

static int handle(netif_t *netif, pkg_t *pkg)
{
    int ret = 0;
    // uint8_t buf[2] = {0x55, 0xAA};
    // package_lseek(pkg, 0);
    // package_write(pkg, buf, 2);
    // 如果数据链路层操作存在，将数据包交给数据链路层处理
    if (netif->link_ops)
    {
        ret = netif->link_ops->in(netif, pkg);
        if (ret < 0)
        {
            // 数据包处理错误，直接丢弃回收
            package_collect(pkg);
            dbg_warning("ether pkg handle fail\r\n");
            return ret;
        }
    }
    else
    {
        // 数据链路层不存在
        if (netif->info.type = NETIF_TYPE_LOOP)
        {
            //loop本身没网卡，没物理地址，没有链路层，link_layer层，因此特殊处理，直接ip层
            ret = ipv4_in(netif,pkg);
            if(ret < 0)
            {
                package_collect(pkg);
            }
        }
        else
        {
            //如果是ether类型，没有link_layer层操作，直接释放包。
            package_collect(pkg);
            return -2;
        }
    }
    return 0;
}
// 从工作台拿东西进行处理
DEFINE_THREAD_FUNC(worker)
{
    int ret;
    soft_timer_list_print();
    while (1)
    {
        uint8_t *buf = NULL;
        uint32_t before = get_cur_time_ms();
        wb_stuff_t *stuff = workbench_get_stuff(soft_timer_get_first_time());
        if (stuff)
        {
            pkg_t *pkg = stuff->package;
            netif_t *netif = stuff->netif;
            workbench_collect_stuff(stuff);
            dbg_info("worker get stuff pakcage,len = %d\n", pkg->total);
            ret = handle(netif, pkg);
        }
        uint32_t after = get_cur_time_ms();
        soft_timer_scan_list(after - before);
        // //数据包处理正确，才可以往out_q里面放
        // if (ret >= 0)
        // {
        //     if (netif) // 这个数据包来自网卡，经过处理，再由该网卡发出
        //     {
        //         /*？？发包的时候记得用ehter_out发送，即netif.link_ops.out，把以太网头封上*/
        //         netif_putpkg(&netif->out_q, pkg);
        //     }
        //     else // 该数据包来自应用程序，并非某个网卡
        //     {
        //     }
        // }
    }
}

#include "net.h"

void networker_start(void)
{
    semaphore_init(&mover_sem, 0);
    thread_create(&netthread_pool, worker, NULL);
    thread_create(&netthread_pool, mover, NULL);
}
