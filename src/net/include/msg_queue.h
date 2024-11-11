#ifndef __MSG_QUEUE_H
#define __MSG_QUEUE_H

#include "net_err.h"
#include "thread.h"



typedef struct _msgQ_t{
    void** msg_que;//空出一个来，头尾相同表示为空
    lock_t locker;
    semaphore_t full;
    semaphore_t empty;

    int capacity;        // 队列容量
    int head;            // 队列头指针
    int tail;            // 队列尾指针
    int size;            // 当前队列大小

}msgQ_t;

net_err_t msgQ_init(msgQ_t* mq,void** q,int capacity);
net_err_t msgQ_destory(msgQ_t* mq);
net_err_t msgQ_enqueue(msgQ_t* mq,void* message,int timeout);
void* msgQ_dequeue(msgQ_t* mq,int timeout);
#endif