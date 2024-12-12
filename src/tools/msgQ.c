#include "msgQ.h"


int msgQ_init(msgQ_t* mq,void** q,int capacity)
{
    mq->capacity = capacity;
    mq->head = 0;
    mq->tail =0;
    mq->size =0;
    mq->msg_que = q;
    

    lock_init(&mq->locker);
    semaphore_init(&mq->full,0);
    semaphore_init(&mq->empty,capacity);

    return 0;
}

int msgQ_destory(msgQ_t* mq)
{
    mq->capacity = 0;
    mq->head = 0;
    mq->tail =0;
    mq->size = 0;
    mq->msg_que = NULL;

    lock_destory(&mq->locker);
    semaphore_destory(&mq->full);
    semaphore_destory(&mq->empty);
    return 0;
}

int msgQ_enqueue(msgQ_t* mq,void* message,int timeout)
{
    int ret;
    ret = time_wait(&mq->empty,timeout);
    if(ret < 0)
    {
        return ret;
    }

    lock(&mq->locker);
    mq->msg_que[mq->tail] = message;
    mq->tail = (mq->tail + 1) % mq->capacity;
    mq->size++;
    unlock(&mq->locker);

    post(&mq->full);
    return 0;
}

void* msgQ_dequeue(msgQ_t* mq,int timeout)
{
    void* message = NULL;
    int ret;
    ret = time_wait(&mq->full,timeout);
    if(ret < 0){
        return NULL;
    }

    lock(&mq->locker);
    message = mq->msg_que[mq->head];
    mq->head = (mq->head + 1) % mq->capacity;
    mq->size--;
    unlock(&mq->locker);

    post(&mq->empty);
    return message;
}

int msgQ_is_empty(msgQ_t *mq)
{

    return mq->size == 0;
}