#include <stdio.h>
#include <unistd.h>
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


int main(int argc, char const *argv[])
{
    uint8_t mac_addr[6]={0x00,0x0c,0x29,0x6e,0x06,0x02};
    // 打开物理网卡，设置好硬件地址
    pcap_t * pcap = pcap_device_open("192.168.128.10", mac_addr);

    while (pcap) {
        static uint8_t buffer[1024];
        static int count = 0;

        printf("begin test: %d\n", count++);

        // 创建待发送的数据
        for (int i = 0; i < sizeof(buffer); i++) {
            buffer[i] = i;
        }

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
