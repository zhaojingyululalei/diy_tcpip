// #include "net_drive.h"
// #include "net/ipaddr.h"
// #include "net/netif.h"
// #include "tools/debug.h"
// #include <unistd.h>
// static uint8_t mac_addr_Host[MAC_ADDR_ARR_LEN] = {0x0, 0x0c, 0x29, 0x6e, 0x06, 0x0c};
// uint8_t *get_mac_addr(const char *name)
// {
//     if (strncmp(name, "ens37", 5) == 0)
//     {
//         return mac_addr_Host;
//     }
//     return NULL;
// }
// void test_drive(void)
// {
//     /*虚拟机新添加了一个网卡,only-host模式， 在netplan中自己进行了配置，命名为ens37*/
//     pcap_show_list();
//     uint8_t *macaddr = get_mac_addr("ens37");
//     pcap_t *pcap = pcap_device_open("192.168.169.10", macaddr);
//     uint8_t *pkg_data;
//     uint8_t buffer[256];
//     int len = 0;
//     while (1)
//     {
//         if ((len = pcap_recv_pkg(pcap, &pkg_data)) > 0)
//         {
//            dbg_info("recv a pkg success\n");
//             len = len > sizeof(buffer) ? sizeof(buffer) : len;
//             memcpy(buffer, pkg_data, len);
//             for (int i = 0; i < len; ++i)
//             {

//                dbg_info("%x ", buffer[i]);
//             }
//            dbg_info("\r\n");
//             buffer[0] = 0x55;
//             buffer[1] = 0xaa;

//             if (pcap_send_pkg(pcap, buffer, len) < 0)
//             {
//                 dbg_error("send a pkg fail\n");
//                 break;
//             }
//         }
//         sleep(2);
//     }

//     return;
// }
// #include "tools/threadpool.h"
// #include <assert.h>
// DEFINE_THREAD_FUNC(test_thread_func) {
//     int* num = (int*)arg;
//    dbg_info("Thread received value: %d\n", *num);
//     *num += 1;
//     return (void*) num;
// }
// #define MAX_THREAD_SIZE 5
// void test_threadpool() {
//     threadpool_attr_t pool_attr = { MAX_THREAD_SIZE }; // Thread pool with 5 threads
//     threadpool_t pool;
//     int result;

//     // Initialize thread pool
//     result = threadpool_init(&pool, &pool_attr);
//     assert(result == 0 && "Thread pool initialization failed!");

//     // Create threads
//     int task_data[] = {1, 2, 3, 4, 5};
//     thread_t* threads[5];
//     for (int i = 0; i < 5; i++) {
//         threads[i] = thread_create(&pool, test_thread_func, &task_data[i]);
//         assert(threads[i] != NULL && "Thread creation failed!");
//     }

//     // Wait for threads to finish
//     for (int i = 0; i < 5; i++) {
//         void* ret;
//         result = thread_join(&pool, threads[i], &ret);
//         assert(result == 0 && "Thread join failed!");
//        dbg_info("Thread %d returned value: %d\n", i + 1, *((int*) ret));
//     }

//     // Verify thread list count
//     int count = threads_list_count(&pool);
//     assert(count == MAX_THREAD_SIZE && "Thread pool is not empty after all threads joined!");

//     // Additional tests: Add more edge cases if necessary

//     // Cleanup and exit
//    dbg_info("Test completed successfully.\n");
// }

// // Test function to test locks (mutex)
// void test_locks() {
//     lock_t locker;
//     int result;

//     // Initialize the lock
//     result = lock_init(&locker);
//     assert(result == 0 && "Lock initialization failed!");

//     // Lock the mutex
//     result = lock(&locker);
//     assert(result == 0 && "Lock failed!");

//     // Simulate some critical section
//    dbg_info("Lock acquired.\n");
//     sleep(1); // Simulate work done in critical section

//     // Unlock the mutex
//     result = unlock(&locker);
//     assert(result == 0 && "Unlock failed!");
//    dbg_info("Lock released.\n");

//     // Destroy the lock
//     result = lock_destory(&locker);
//     assert(result == 0 && "Lock destruction failed!");

//    dbg_info("Lock test completed successfully.\n");
// }

// // Test function to test semaphores
// void test_semaphores() {
//     semaphore_t sem;
//     int result;

//     // Initialize the semaphore with a count of 2
//     result = semaphore_init(&sem, 2);
//     assert(result == 0 && "Semaphore initialization failed!");

//     // Post (signal) the semaphore twice (to increase count)
//     result = post(&sem);
//     assert(result == 0 && "Post operation failed!");
//     result = post(&sem);
//     assert(result == 0 && "Post operation failed!");

//     // Wait (decrement) the semaphore, it should block until the count is available
//     result = wait(&sem);
//     assert(result == 0 && "Wait operation failed!");
//    dbg_info("Semaphore wait succeeded, count decremented.\n");

//     // Wait with timeout (non-blocking wait)
//     result = time_wait(&sem, 0);
//     assert(result == 0 && "Timeout wait failed!");

//     // Wait with a delay
//     result = time_wait(&sem, 2);  // Timeout > 0 should wait
//     assert(result == 0 && "Wait with timeout failed!");

//     // Destroy the semaphore
//     result = semaphore_destory(&sem);
//     assert(result == 0 && "Semaphore destruction failed!");

//    dbg_info("Semaphore test completed successfully.\n");
// }
// #include "mmpool.h"
// void test_mempool() {
//     // 设置内存池参数
//     uint8_t buffer[1024];  // 假设总共1024字节的内存池
//     int block_limit = 10;   // 最多有10个块
//     int block_size = 100;   // 每个块的大小是100字节

//     mempool_t mempool;
//     int result;

//     // 初始化内存池
//     result = mempool_init(&mempool, buffer, block_limit, block_size);
//     assert(result == 0 && "内存池初始化失败!");

//     // 检查空闲块数
//     uint32_t free_count = mempool_freeblk_cnt(&mempool);
//     assert(free_count == block_limit && "初始化时内存池的空闲块数不正确!");

//     // 分配内存块
//     void* blk1 = mempool_alloc_blk(&mempool, 0);  // 无超时
//     assert(blk1 != NULL && "内存块分配失败!");

//     free_count = mempool_freeblk_cnt(&mempool);
//     assert(free_count == block_limit - 1 && "分配内存块后空闲块数不正确!");

//     void* blk2 = mempool_alloc_blk(&mempool, 0);  // 无超时
//     assert(blk2 != NULL && "第二块内存分配失败!");

//     free_count = mempool_freeblk_cnt(&mempool);
//     assert(free_count == block_limit - 2 && "分配第二块内存后空闲块数不正确!");

//     // 释放一个内存块
//     result = mempool_free_blk(&mempool, blk1);
//     assert(result == 0 && "释放内存块失败!");

//     free_count = mempool_freeblk_cnt(&mempool);
//     assert(free_count == block_limit - 1 && "释放内存块后空闲块数不正确!");

//     // 分配一个块，验证超时功能
//     void* blk3 = mempool_alloc_blk(&mempool, 1);  // 超时1秒
//     assert(blk3 != NULL && "超时内存分配失败!");

//     free_count = mempool_freeblk_cnt(&mempool);
//     assert(free_count == block_limit - 2 && "超时分配块后空闲块数不正确!");

//     // 销毁内存池
//     mempool_destroy(&mempool);
//    dbg_info("内存池测试完成。\n");
// }
// #include "msgQ.h"
// void test_msg_queue() {
//     int capacity = 5;  // 队列的容量为5
//     void* queue[capacity];  // 存储消息队列的缓冲区
//     msgQ_t mq;
//     int result;

//     // 初始化消息队列
//     result = msgQ_init(&mq, queue, capacity);
//     assert(result == 0 && "消息队列初始化失败!");

//     // 检查队列是否为空
//     int is_empty = msgQ_is_empty(&mq);
//     assert(is_empty == 1 && "队列初始化时不为空!");

//     // 向队列中入队
//     char* msg1 = "Message 1";
//     result = msgQ_enqueue(&mq, msg1, 0);  // 无超时
//     assert(result == 0 && "入队失败!");

//     is_empty = msgQ_is_empty(&mq);
//     assert(is_empty == 0 && "入队后队列应不为空!");

//     // 向队列中入队第二个消息
//     char* msg2 = "Message 2";
//     result = msgQ_enqueue(&mq, msg2, 0);  // 无超时
//     assert(result == 0 && "入队失败!");

//     // 从队列中出队
//     void* dequeued_msg = msgQ_dequeue(&mq, 0);  // 无超时
//     assert(dequeued_msg != NULL && "出队失败!");
//    dbg_info("出队的消息: %s\n", (char*)dequeued_msg);
//     assert(dequeued_msg == msg1 && "出队的消息不正确!");

//     // 检查队列的状态
//     is_empty = msgQ_is_empty(&mq);
//     assert(is_empty == 0 && "出队后队列不为空!");

//     // 从队列中出队第二个消息
//     dequeued_msg = msgQ_dequeue(&mq, 0);  // 无超时
//     assert(dequeued_msg != NULL && "出队失败!");
//    dbg_info("出队的消息: %s\n", (char*)dequeued_msg);
//     assert(dequeued_msg == msg2 && "出队的消息不正确!");

//     // 检查队列是否为空
//     is_empty = msgQ_is_empty(&mq);
//     assert(is_empty == 1 && "队列应该为空!");

//     // 入队超过队列容量的情况
//     char* msg3 = "Message 3";
//     for (int i = 0; i < capacity; i++) {
//         result = msgQ_enqueue(&mq, msg3, -1);  // 无超时
//         assert(result == 0 && "入队失败!");
//     }
//     result = msgQ_enqueue(&mq, msg3, -1);  // 队列已满，应该失败
//     assert(result != 0 && "队列已满时应无法入队!");

//     // 销毁消息队列
//     result = msgQ_destory(&mq);
//     assert(result == 0 && "销毁消息队列失败!");

//    dbg_info("消息队列测试完成。\n");
// }

/*测试数据包接口*/
// 用于初始化包池和打印信息
#include "stdio.h"
#include "package.h"
void init_and_print_pool_info()
{

    package_pool_init();

    package_show_pool_info();
}

// 测试创建包和添加头信息

void test_create_and_add_header()
{

    uint8_t data_buf[256] = {0}; // 示例数据缓冲区

    pkg_t *package = package_create(data_buf, sizeof(data_buf));

    if (!package)
    {

        fprintf(stderr, "Failed to create package\n");

        return;
    }

    uint8_t head_buf[16] = "HEADER_DATA"; // 示例头信息

    if (package_add_header(package, head_buf, sizeof(head_buf)) != 0)
    {

        fprintf(stderr, "Failed to add header\n");

        package_alloc(0); // 释放包（假设这是释放包的正确方式，具体根据实现）

        return;
    }

    package_show_info(package);
}

// 测试写入、读取和内存操作

void test_write_read_memory_ops()
{
    pkg_t* pkg = package_alloc(131);
    package_show_info(pkg);
    package_memset(pkg,0,0,pkg->total);
    package_write_pos(pkg,"hello",5,0);
    package_write_pos(pkg,"world",5,128);
    package_write_pos(pkg,"nihao",5,256);
    package_write_pos(pkg,"hehed",5,384);
    package_shrank_last(pkg,381);
    char* ret = package_data(pkg,5,128);
    printf("ret= %s\r\n",ret);
}
void test_package(void)
{
    init_and_print_pool_info();
    test_create_and_add_header();
    test_write_read_memory_ops();
    package_pool_destory();
}
// #include "ipaddr.h"
// void test_ipaddr(void)
// {
//     char buf[16] = {0};
//     ipaddr_t ip;
//     ipaddr_s2n("192.168.1.25",&ip);
//     ipaddr_n2s(&ip,buf,16);
// }
// #include "networker.h"
// #include "net.h"
// #include "pkg_workbench.h"
// void app_(void)
// {
//     netif_t* netcard = malloc(sizeof(netif_t));

// }
#include "networker.h"
#include "pkg_workbench.h"
#include "threadpool.h"
#include "loop.h"
#include "net.h"
DEFINE_THREAD_FUNC(app)
{
    while (1)
    {
        // 这里模拟应用程序不断向workbench发送数据包
        char *tmp = "app test";
        pkg_t *app_pkg = package_create(tmp, strlen(tmp));
        workbench_put_stuff(app_pkg, NULL);
        sleep(20); // 主进程不退出，子线程都是死循环，回收不了
    }
}
void test_loop(void)
{
    netif_t *netif = loop_init();
    netif_open(netif);
    print_netif_list();
    netif_activate(netif);

    thread_create(&netthread_pool, app, NULL);
    sleep(3);
    netif_shutdown(netif); // 主线程等在回收recv线程那里，卡死
    dbg_info("+++++++++++++++++++++++++shutdown loop++++++++++++++++\r\n");
    sleep(3);
    dbg_info("+++++++++++++++++++++++++activate loop++++++++++++++++\r\n");
    netif_activate(netif);

    while (1)
    {
        sleep(1);
    }
}

#include "soft_timer.h"
DEFINE_TIMER_FUNC(timer_0_handle)
{
    int *x = (int *)arg;
    int *ret = malloc(sizeof(int));
    *ret = *x + 10;
    dbg_info("in timer handler .. arg:%d  ret:%d\r\n", *x, *ret);
    return ret;
}
DEFINE_TIMER_FUNC(timer_1_handle)
{
    int *x = (int *)arg;
    int *ret = malloc(sizeof(int));
    *ret = *x + 10;
    dbg_info("in timer handler .. arg:%d  ret:%d\r\n", *x, *ret);
    return ret;
}
DEFINE_TIMER_FUNC(timer_2_handle)
{
    int *x = (int *)arg;
    int *ret = malloc(sizeof(int));
    *ret = *x + 10;
    dbg_info("in timer handler .. arg:%d  ret:%d\r\n", *x, *ret);
    return ret;
}
DEFINE_TIMER_FUNC(timer_3_handle)
{
    int *x = (int *)arg;
    int *ret = malloc(sizeof(int));
    *ret = *x + 10;
    dbg_info("in timer handler .. arg:%d  ret:%d\r\n", *x, *ret);
    return ret;
}
#include "timer.h"
soft_timer_t timer_0, timer_1, timer_2, timer_3;
int a = 0, b = 1, c = 2, d = 3;
int *ret0 = NULL;
int *ret1 = NULL;
int *ret2 = NULL;
int *ret3 = NULL;
void timer_test(void)
{
    // uint32_t before = get_cur_time_ms();
    // sleep(1);
    // uint32_t after = get_cur_time_ms();
    // uint32_t ret = after - before;

    soft_timer_add(&timer_0, SOFT_TIMER_TYPE_ONCE, 100, "timer_0", timer_0_handle, &a, &ret0);
    soft_timer_add(&timer_2, SOFT_TIMER_TYPE_PERIOD, 8000, "timer_2", timer_2_handle, &c, &ret2);
    soft_timer_add(&timer_1, SOFT_TIMER_TYPE_PERIOD, 1000, "timer_1", timer_1_handle, &b, &ret1);
    soft_timer_add(&timer_3, SOFT_TIMER_TYPE_PERIOD, 2000, "timer_3", timer_3_handle, &d, &ret3);
    soft_timer_list_print();
    // //int diff_ms = 0;
    // while (1)
    // {
    //     // dbg_info("*ret0 =%d\r\n", ret0?*ret0:-1);
    //     // dbg_info("*ret1 =%d\r\n", ret1?*ret1:-1);
    //     // dbg_info("*ret2 =%d\r\n", ret2?*ret2:-1);
    //     sleep(1);
    //     //diff_ms+=1000;
    //     soft_timer_scan_list(1000);
    // }

    // soft_timer_remove(&timer_1);
    // soft_timer_list_print();
    // soft_timer_remove(&timer_2);
    // soft_timer_list_print();
}
#include "phnetif.h"
#include "arp.h"
void test_phnetif(void)
{
    netif_t *netif = phnetif_init();
    netif_open(netif);
    print_netif_list();
    netif_activate(netif);
    ipaddr_t ip = {
        .type = IPADDR_V4};
    char *ip_buf = "192.168.169.20";
    ipaddr_s2n(ip_buf, &ip);
    ipaddr_t mask = {
        .type = IPADDR_V4};
    char *mask_buf = "255.255.255.0";
    ipaddr_s2n(mask_buf, &mask);

    for (int i = 0; i < 2; ++i)
    {
        uint8_t data_buf[2] = {0x55, 0xAA};
        pkg_t *pkg = package_create(data_buf, 2);
        // arp_send_request(netif,&ip);
        netif_out(netif, &ip,  pkg);
    }

    
}

//ipv4 test
#include "ipv4.h"
#include "protocal.h"
void test_ipv4(void)
{
    // netif_t *netif = loop_init();
    // netif_open(netif);
    // print_netif_list();
    // netif_activate(netif);
    netif_t *netif = phnetif_init();
    netif_open(netif);
    print_netif_list();
    netif_activate(netif);
    ipaddr_t srcip = {
        .type = IPADDR_V4};
    char *srcip_buf = "192.168.169.10";
    ipaddr_s2n(srcip_buf, &srcip);

    ipaddr_t destip = {
        .type = IPADDR_V4};
    char *destip_buf = "192.168.169.20";
    ipaddr_s2n(destip_buf, &destip);

    uint8_t data_buf[2] = {0x55, 0xAA};
    pkg_t *pkg = package_create(data_buf, 2);
    //ipv4_out(pkg,PROTOCAL_TYPE_ICMPV4,&srcip,&destip);
    
    
    
    
    
    return;

}
void test_worker(void)
{
    net_system_init();
    // timer_test();
    net_system_start();
    //test_phnetif();
    test_ipv4();
    while (1)
    {
        sleep(1);
    }

}
int main(int agrc, char *argv[])
{
    // test_drive();
    //   test_threadpool();
    //   test_locks();
    //   test_semaphores();
    //  test_mempool();
     //test_package();
    //  test_ipaddr();
    // timer_test();
    test_worker();
    return 0;
}