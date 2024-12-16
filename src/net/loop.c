#include "loop.h"
#include "netif.h"
#include "debug.h"

#define QUEUE_CAPACITY 1024

typedef struct {
    uint8_t* data; // 队列存储空间
    uint16_t head;                // 队列头索引
    uint16_t tail;                // 队列尾索引
    uint16_t size;                // 当前队列大小
} CircularQueue;
static CircularQueue queue;
// 初始化队列
static void initQueue(CircularQueue* queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
    queue->data = malloc(QUEUE_CAPACITY);
}
static void deinitQueue(CircularQueue* queue)
{
    free(queue->data);
}

// 检查队列是否为空
static int isQueueEmpty(CircularQueue* queue) {
    return queue->size == 0;
}

// 检查队列是否已满
static int isQueueFull(CircularQueue* queue) {
    return queue->size == QUEUE_CAPACITY;
}

// 入队操作
static  int enqueue(CircularQueue* queue, uint8_t value) {
    if (isQueueFull(queue)) {
        return 0; // 队列已满，无法入队
    }
    queue->data[queue->tail] = value;
    queue->tail = (queue->tail + 1) % QUEUE_CAPACITY; // 环形移动
    queue->size++;
    return 1;
}

// 出队操作
static int dequeue(CircularQueue* queue, uint8_t* value) {
    if (isQueueEmpty(queue)) {
        return 0; // 队列为空，无法出队
    }
    *value = queue->data[queue->head];
    queue->head = (queue->head + 1) % QUEUE_CAPACITY; // 环形移动
    queue->size--;
    return 1;
}

// 获取队列的当前大小
static uint16_t getQueueSize(CircularQueue* queue) {
    return queue->size;
}

static const netif_ops_t loop_ops={
    .open = loop_open,
    .close = loop_close,
    .receive = loop_receive,
    .send = loop_send
};

netif_t* loop_init(void)
{
    
    netif_info_t *loop_info= (netif_info_t *)malloc(sizeof(netif_info_t));
    memset(loop_info,0,sizeof(netif_info_t));
    ipaddr_s2n(NETIF_LOOP_IPADDR,&loop_info->ipaddr);
    ipaddr_s2n(NETIF_LOOP_MASK,&loop_info->mask);
    strncpy(loop_info->name,"lo",2);


    netif_t* ret = netif_virtual_register(loop_info,&loop_ops,NULL);
    if(ret == NULL)
    {
        free(loop_info);
        dbg_error("loop init fail\r\n");
        return NULL;
    }
    
    
    return ret;
}

/*算是虚拟网卡，没有驱动，因此为空实现*/
int loop_open(netif_t* netif,void* ex_data)
{
    if(!netif)
    {
        return -1;
    }
    netif->info.type = NETIF_TYPE_LOOP;
    initQueue(&queue);

    return 0;

}
int loop_close(netif_t* netif)
{
    deinitQueue(&queue);
    return 0;
}

int loop_send(netif_t* netif,const uint8_t* buf,int len)
{
    int ret;
    for (int i = 0; i < len; i++)
    {
        ret = enqueue(&queue,buf[i]);
        if(!ret)
        {
            return i;
        }
    }
    
    return len;
}

int loop_receive(netif_t* netif,uint8_t* buf,int len)
{
    int ret,i;
    for (i = 0; i < len; i++)
    {
        ret = dequeue(&queue,&buf[i]);
        if(!ret)
        {
            return i;
        }
    }
    return len;
}
