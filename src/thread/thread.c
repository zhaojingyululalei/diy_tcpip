#include "thread.h"
#include <errno.h>
#include <time.h>

list_t threads_list;

net_err_t threads_init(void)
{
    list_init(&threads_list);
    for(int i = 0;i<MAX_THREADS_NR;++i)
    {
        thread_t* new = (thread_t*)malloc(sizeof(thread_t));
        list_node_init(&new->node);
        list_insert_last(&threads_list,&new->node);
    }
    return NET_ERR_OK;
}

thread_t* thread_create(thread_func_t func,void* func_arg)
{
    if(list_count(&threads_list)==0)
    {
        printf("there is no more thread\n");
        return NULL;
    }
    list_node_t* tnode = list_remove_first(&threads_list);
    thread_t* thread = list_node_parent(tnode,thread_t,node);
    if(pthread_create(&thread->thread,NULL,func,func_arg) != 0)
    {
        perror("Failed to create thread 2");
        return NULL;
    }
    return thread;
}

net_err_t thread_join(thread_t* thread,void** ret)
{
    if(pthread_join(thread->thread,ret)!=0)
    {
        perror("Failed to join thread 1");
        return NET_ERR_SYS;
    }
    list_insert_last(&threads_list,thread);
    return NET_ERR_OK;
}

int threads_list_count(void)
{
    return list_count(&threads_list);
}

net_err_t lock(lock_t* lk)
{
    int ret;
    ret =  pthread_mutex_lock(lk);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
}

net_err_t unlock(lock_t* lk)
{
    int ret;
    ret =  pthread_mutex_unlock(lk);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
}

net_err_t lock_init(lock_t* lk)
{
    int ret;
    ret = pthread_mutex_init(lk, NULL);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
}

net_err_t lock_destory(lock_t* lk)
{
    int ret;
    ret = pthread_mutex_destroy(lk);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
}

net_err_t semaphore_init(semaphore_t* sem,uint32_t count)
{
    int ret;
    ret = sem_init(sem,0,count);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
}
net_err_t semaphore_destory(semaphore_t* sem)
{
    int ret;
    ret = sem_destroy(sem);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
}

net_err_t wait(semaphore_t* sem)
{
    int ret;
    ret = sem_wait(sem);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
}
#define CLOCK_REALTIME	0
/*毫秒*/
net_err_t time_wait(semaphore_t *sem, int timeout) {
    if (timeout < 0) {
        // timeout < 0 时，尝试非阻塞地获取信号量
        int ret = sem_trywait(sem);
        if (ret == 0) {
            return NET_ERR_OK;  // 成功获取信号量
        } else {
            return NET_ERR_TIMEOUT;  // 没有资源，立即返回
        }
    } else if (timeout == 0) {
        // timeout == 0 时，一直阻塞等待直到获取到信号量
        int ret = sem_wait(sem);
        return ret == 0 ? NET_ERR_OK : NET_ERR_SYS;
    } else {
        // timeout > 0 时，设置超时时间
        struct timespec ts;

        // 获取当前时间
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            return NET_ERR_SYS;
        }

        // 设置绝对超时时间
        ts.tv_sec += timeout / 1000;              // 秒部分
        ts.tv_nsec += (timeout % 1000) * 1000000; // 毫秒部分转为纳秒
        if (ts.tv_nsec >= 1000000000) {           // 如果纳秒超过1秒，进位到秒
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        // 等待信号量，超时返回
        int ret = sem_timedwait(sem, &ts);
        if (ret == 0) {
            return NET_ERR_OK;  // 成功获取信号量
        } else if (errno == ETIMEDOUT) {
            return NET_ERR_TIMEOUT;  // 超时返回
        } else {
            return NET_ERR_SYS;  // 其他错误
        }
    }
}

net_err_t post(semaphore_t* sem)
{
    int ret;
    ret = sem_post(sem);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
    
}

