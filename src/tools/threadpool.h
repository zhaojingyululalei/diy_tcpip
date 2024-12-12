#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H
#include "list.h"
#include "types.h"
// 定义线程函数宏，
#define DEFINE_THREAD_FUNC(name)            void* name(void* arg)
typedef void*(*thread_func_t)(void*);

typedef struct _thread_t{
    pthread_t thread;
    list_node_t node;
}thread_t;

typedef struct _threadpool_attr_t
{
    int threads_nr;
}threadpool_attr_t;
typedef struct _threadpool_t
{
    list_t list;
    thread_t* threads_buf;
    threadpool_attr_t* attr;
}threadpool_t;


int threadpool_init(threadpool_t* pool,threadpool_attr_t* attr);
thread_t* thread_create(threadpool_t* pool,thread_func_t func,void* func_arg);
int thread_join(threadpool_t* pool,thread_t* thread,void** ret);
int threads_list_count(threadpool_t* pool);

typedef pthread_mutex_t lock_t;
int lock(lock_t* lk);
int unlock(lock_t* lk);
int lock_init(lock_t* lk);
int lock_destory(lock_t* lk);

typedef sem_t semaphore_t;
int semaphore_init(semaphore_t* sem,uint32_t count);
int semaphore_destory(semaphore_t* sem);
int wait(semaphore_t* sem);
/**
 * @return timeout<0 非阻塞，立即返回  timeout==0阻塞死等  timeout>0等够时间返回
 */
int time_wait(semaphore_t* sem, int timeout);
int post(semaphore_t* sem);

#endif
