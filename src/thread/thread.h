#ifndef __THREAD_H
#define __THREAD_H



#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include "tools/list.h"
#include "net_err.h"
#define MAX_THREADS_NR  256
// 定义线程函数宏，
#define DEFINE_THREAD_FUNC(name)            void* name(void* arg)
typedef void*(*thread_func_t)(void*);

typedef struct _thread_t{
    pthread_t thread;
    list_node_t node;
}thread_t;


net_err_t threads_init(void);
thread_t* thread_create(thread_func_t func,void* func_arg);
net_err_t thread_join(thread_t* thread,void** ret);
int threads_list_count(void);

typedef pthread_mutex_t lock_t;
net_err_t lock(lock_t* lk);
net_err_t unlock(lock_t* lk);
net_err_t lock_init(lock_t* lk);
net_err_t lock_destory(lock_t* lk);

typedef sem_t semaphore_t;
net_err_t semaphore_init(semaphore_t* sem,uint32_t count);
net_err_t semaphore_destory(semaphore_t* sem);
net_err_t wait(semaphore_t* sem);
net_err_t post(semaphore_t* sem);
#endif

