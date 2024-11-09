#include "thread.h"

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
    ret = pthread_mutex_init(&lk, NULL);
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

net_err_t post(semaphore_t* sem)
{
    int ret;
    ret = sem_post(sem);
    return ret==0?NET_ERR_OK:NET_ERR_SYS;
    
}

