#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "thread/thread.h"
#include "dev/pnet.h"




void test02()
{
    pcap_show_list();
}
void test01()
{
    int ret = 0;
    char buff1[256] = {0};
    ret = pcap_find_device("192.168.128.1", buff1);
    if (ret == -1)
    {
        printf("cant find\n");
    }
    else
    {

        printf("%s\n", buff1);
    }

    char buff2[256] = {0};
    ret = pcap_find_device("192.168.128.2", buff2);
    if (ret == -1)
    {
        printf("cant find\n");
    }
    else
    {

        printf("%s\n", buff2);
    }

    char buff3[256] = {0};
    ret = pcap_find_device("192.168.128.10", buff3);
    if (ret == -1)
    {
        printf("cant find\n");
    }
    else
    {

        printf("%s\n", buff3);
    }

}


uint8_t mac_addr_NAT[6]={0x00,0x0c,0x29,0x6e,0x06,0x02};
uint8_t mac_addr_Host[6] = {0x0,0x0c,0x29,0x6e,0x06,0x0c};
void test03()
{
    // 打开物理网卡，设置好硬件地址
    pcap_t * pcap = pcap_device_open("192.168.169.10", mac_addr_Host);

    while (pcap) {
        static uint8_t buffer[1024];
        static int count = 0;
        struct pcap_pkthdr* pkthdr;
        const uint8_t* pkt_data;
        printf("begin test: %d\n", count++);

        // 以下测试程序，读取网络上的广播包，然后再发送出去.
        // 读取数据包
        if (pcap_next_ex(pcap, &pkthdr, &pkt_data) != 1) {
            continue;
        }
        
        // 稍微修改一下，再发送
        int len = pkthdr->len > sizeof(buffer) ? sizeof(buffer) : pkthdr->len;
        memcpy(buffer, pkt_data, len);
        buffer[0] = 0x55;
        buffer[1] = 0xaa;
        

        // 发送数据包
        if (pcap_inject(pcap, buffer, sizeof(buffer)) == -1) {
            printf("pcap send: send packet failed!:%s\n", pcap_geterr(pcap));
            printf("pcap send: pcaket size %d\n", (int)sizeof(buffer));
            break;
        }

        // 延时一会儿，避免CPU负载过高
        sleep(1);
        printf("hello");
    }
}


// 线程函数
void* thread_function(void* arg) {
    int thread_num = *((int*)arg); // 将传入的参数转换为线程编号
    printf("Thread %d: Starting\n", thread_num);

    // 模拟线程工作
    sleep(2); // 暂停1秒，模拟工作

    // 返回一个动态分配的整数结果
    int* result = malloc(sizeof(int));
    if (result == NULL) {
        perror("Failed to allocate memory");
        pthread_exit(NULL);
    }
    *result = thread_num * 10; // 假设返回的值是线程编号乘以 10

    printf("Thread %d: Finished, returning %d\n", thread_num, *result);
    return result; // 返回结果
}


#include "net.h"
#include "net/include/netif.h"
#include "debug.h"
#include "mempool.h"

void pool_test(void)
{
    uint8_t buffer[1025*512];
    mempool_t pool;
    void* tmp[20];
    mempool_init(&pool,buffer,1024,512);
    for (int i = 0; i < 20; i++)
    {
        void* blk = mempool_alloc_blk(&pool,500);
        tmp[i] = blk;
        dbg_info("blk address is :%x. free list count is %d\n",(uint32_t)blk,mempool_freeblk_cnt(&pool));
    }
    for (int i = 0; i < 20; i++)
    {
        mempool_free_blk(&pool,tmp[i]);
        dbg_info("free list count is %d\n",mempool_freeblk_cnt(&pool));
    }
}
#include "include/package.h"
void pkg_test(void)
{
    printf("create a pkg\n");
    pkg_t *p = package_create(520);
    package_show_info(p);
    package_show_pool_info();

    printf("pkg expand front  20\n");
    package_expand_front(p,20);
    package_show_info(p);
    package_show_pool_info();

    printf("pkg expand front  100\n");
    package_expand_front(p,100);
    package_show_info(p);
    package_show_pool_info();

    printf("pkg expand last 30\n");
    package_expand_last(p,30);
    package_show_info(p);
    package_show_pool_info();

    printf("pkg expand last 100\n");
    package_expand_last(p,100);
    package_show_info(p);
    package_show_pool_info();

    printf("add header size 4\n");
    package_add_header(p,4);
    package_show_info(p);
    package_show_pool_info();

    printf("add header size 20\n");
    package_add_header(p,20);
    package_show_info(p);
    package_show_pool_info();
    
    printf("integrate header size 24");
    package_integrate_header(p,24);
    package_show_info(p);
    package_show_pool_info();


    package_collect(p);
    package_show_pool_info();
}
int main(int argc,char* argv[]) {
    net_init();
    net_start(); //协议栈开始工作，消息队列开始工作
    
    netif_open(); //网卡收发线程开始工作
    pkg_test();
    
    while (1)
    {
        sleep(1);
    }
    
    return 0;
    
}




