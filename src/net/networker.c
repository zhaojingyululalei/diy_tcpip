#include "networker.h"
#include "pkg_workbench.h"
#include "threadpool.h"
static threadpool_t netthread_pool;
// 数据包搬运工,采集网卡数据,放入工作台
DEFINE_THREAD_FUNC(mover)
{
    while (1)
    {

        char *buf = "hello";
        pkg_t *package = package_create(buf, strlen(buf));
        sleep(1);

        workbench_put_stuff(package); // 放入工作台
        dbg_info("mover put a stuff on workbench\n");
    }
}

//从工作台拿东西进行处理
DEFINE_THREAD_FUNC(worker)
{
    while (1)
    {
        char buf[64] ={0};
        pkg_t* package = workbench_get_stuff();
        int len = package_read(package,buf,package_get_total(package));
        dbg_info("worker get stuff pakcage,len = %d\n",len);
        sleep(1);
    }
    
}

void networker_start(void)
{
    package_pool_init();

    workbench_init();

    threadpool_attr_t pool_attr;
    pool_attr.threads_nr = THREADPOOL_THREADS_NR;
    threadpool_init(&netthread_pool,&pool_attr);

    thread_create(&netthread_pool,worker,NULL);
    thread_create(&netthread_pool,mover,NULL);

    while (1)
    {
        sleep(1);//主进程不退出，子线程都是死循环，回收不了
    }
    

}
